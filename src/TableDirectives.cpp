// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Guid.h"
#include "Types.h"

using namespace std::literals;

namespace graphql::mapi {

namespace {

template <typename T, service::TypeModifier... Modifiers>
using modified_type_t =
	typename service::ModifiedArgument<T>::template ArgumentTraits<T, Modifiers...>::type;

template <typename T, service::TypeModifier... Modifiers>
std::optional<modified_type_t<T, Modifiers...>> GetFieldDirectiveArgument(
	std::string_view directiveName, std::string_view argumentName,
	const service::Directives& fieldDirectives)
{
	const auto itr = std::find_if(fieldDirectives.begin(),
		fieldDirectives.end(),
		[directiveName](const auto& entry) noexcept {
			return entry.first == directiveName;
		});

	if (itr == fieldDirectives.end())
	{
		// Return std::nullopt the directive was not found.
		return std::nullopt;
	}

	// The argument is required if the directive is present.
	return std::make_optional(
		service::ModifiedArgument<T>::template require<Modifiers...>(argumentName, itr->second));
}

} // namespace

TableDirectives::TableDirectives(
	const std::shared_ptr<Store>& store, const service::Directives& fieldDirectives) noexcept
	: m_store { store }
	, m_columns { GetFieldDirectiveArgument<Column, service::TypeModifier::List>(
		  "columns"sv, "ids"sv, fieldDirectives) }
	, m_orderBy { GetFieldDirectiveArgument<Order, service::TypeModifier::List>(
		  "orderBy"sv, "sorts"sv, fieldDirectives) }
	, m_seek { GetFieldDirectiveArgument<response::IdType, service::TypeModifier::Nullable>(
		  "seek"sv, "id"sv, fieldDirectives) }
	, m_offset { GetFieldDirectiveArgument<int>("offset"sv, "count"sv, fieldDirectives) }
	, m_take { GetFieldDirectiveArgument<int>("take"sv, "count"sv, fieldDirectives) }
{
	if (m_store && m_columns && !m_columns->empty() && m_orderBy && !m_orderBy->empty())
	{
		// Pre-resolve all of the named properties in the columns and orderBy directives together in
		// a single call
		std::vector<PropIdInput> propIds;

		propIds.reserve(m_columns->size() + m_orderBy->size());
		std::transform(m_columns->cbegin(),
			m_columns->cend(),
			std::back_insert_iterator(propIds),
			[](const auto& column) noexcept {
				return column.property;
			});
		std::transform(m_orderBy->cbegin(),
			m_orderBy->cend(),
			std::back_insert_iterator(propIds),
			[](const auto& order) noexcept {
				return order.property;
			});

		auto resolved = m_store->lookupPropIdInputs(std::move(propIds));
	}
}

rowset_ptr TableDirectives::read(IMAPITable* pTable, mapi_ptr<SPropTagArray>&& defaultColumns,
	mapi_ptr<SSortOrderSet>&& defaultOrder) const
{
	rowset_ptr result;
	const auto defaultColumnCount =
		static_cast<size_t>(defaultColumns ? defaultColumns->cValues : 0);
	const auto properties = columns(std::move(defaultColumns));
	const auto sorts = orderBy(std::move(defaultOrder));
	const auto findRow = seek();
	const BOOKMARK bookmark = seekBookmark();

	CORt(pTable->SetColumns(properties.get(), TBL_BATCH));

	if (sorts)
	{
		CORt(pTable->SortTable(sorts.get(), TBL_BATCH));
	}

	if (findRow)
	{
		CORt(pTable->FindRow(findRow.get(), BOOKMARK_BEGINNING, 0));
	}

	CORt(pTable->SeekRow(bookmark, offset(), nullptr));
	CORt(pTable->QueryRows(take(), 0, &out_ptr { result }));

	return result;
}

mapi_ptr<SPropTagArray> TableDirectives::columns(mapi_ptr<SPropTagArray>&& defaultColumns) const
{
	auto result = std::move(defaultColumns);

	if (m_columns && !m_columns->empty())
	{
		mapi_ptr<SPropTagArray> mergedColumns;
		const size_t defaultCount = result ? static_cast<size_t>(result->cValues) : 0;

		CORt(::MAPIAllocateBuffer(CbNewSPropTagArray(defaultCount + m_columns->size()),
			reinterpret_cast<void**>(&out_ptr { mergedColumns })));
		CFRt(mergedColumns != nullptr);
		mergedColumns->cValues = static_cast<ULONG>(defaultCount + m_columns->size());
		std::copy(result->aulPropTag, result->aulPropTag + defaultCount, mergedColumns->aulPropTag);

		std::vector<PropIdInput> propIds(m_columns->size());

		std::transform(m_columns->cbegin(),
			m_columns->cend(),
			propIds.begin(),
			[](const auto& column) noexcept {
				return column.property;
			});

		std::vector<std::pair<ULONG, LPMAPINAMEID>> resolved;

		if (m_store)
		{
			resolved = m_store->lookupPropIdInputs(std::move(propIds));
		}
		else
		{
			resolved.resize(m_columns->size());
			std::transform(m_columns->cbegin(),
				m_columns->cend(),
				resolved.begin(),
				[](const Column& column) {
					// Can't use named properties without a store to call GetIDsFromNames
					CFRt(column.property.id && !column.property.named);
					return std::make_pair(PROP_TAG(PT_UNSPECIFIED, *column.property.id), nullptr);
				});
		}

		for (size_t i = 0; i < resolved.size(); ++i)
		{
			constexpr std::array c_propTypes {
				PT_LONG,
				PT_BOOLEAN,
				PT_UNICODE,
				PT_CLSID,
				PT_SYSTIME,
				PT_BINARY,
			};

			CFRt(static_cast<size_t>(m_columns->at(i).type) < c_propTypes.size());

			const auto propType = c_propTypes[static_cast<size_t>(m_columns->at(i).type)];
			const auto propId = PROP_ID(resolved[i].first);

			mergedColumns->aulPropTag[defaultCount + i] = PROP_TAG(propType, propId);
		}

		result = std::move(mergedColumns);
	}

	return result;
}

mapi_ptr<SSortOrderSet> TableDirectives::orderBy(mapi_ptr<SSortOrderSet>&& defaultOrder) const
{
	auto result = std::move(defaultOrder);

	if (m_orderBy && !m_orderBy->empty())
	{
		std::vector<PropIdInput> propIds(m_orderBy->size());

		std::transform(m_orderBy->cbegin(),
			m_orderBy->cend(),
			propIds.begin(),
			[](const auto& order) noexcept {
				return order.property;
			});

		std::vector<std::pair<ULONG, LPMAPINAMEID>> resolved;

		if (m_store)
		{
			resolved = m_store->lookupPropIdInputs(std::move(propIds));
		}
		else
		{
			resolved.resize(m_orderBy->size());
			std::transform(m_orderBy->cbegin(),
				m_orderBy->cend(),
				resolved.begin(),
				[](const Order& order) {
					// Can't use named properties without a store to call GetIDsFromNames
					CFRt(order.property.id && !order.property.named);
					return std::make_pair(PROP_TAG(PT_UNSPECIFIED, *order.property.id), nullptr);
				});
		}

		CFRt(resolved.size() == m_orderBy->size());
		CORt(::MAPIAllocateBuffer(CbNewSSortOrderSet(resolved.size()),
			reinterpret_cast<void**>(&out_ptr { result })));
		CFRt(result != nullptr);
		result->cSorts = static_cast<ULONG>(resolved.size());
		result->cCategories = 0;
		result->cExpanded = 0;

		for (size_t i = 0; i < resolved.size(); ++i)
		{
			constexpr std::array c_propTypes {
				PT_LONG,
				PT_BOOLEAN,
				PT_UNICODE,
				PT_CLSID,
				PT_SYSTIME,
				PT_BINARY,
			};

			CFRt(static_cast<size_t>(m_orderBy->at(i).type) < c_propTypes.size());

			const auto propType = c_propTypes[static_cast<size_t>(m_orderBy->at(i).type)];
			const auto propId = PROP_ID(resolved[i].first);

			result->aSort[i].ulPropTag = PROP_TAG(propType, propId);
			result->aSort[i].ulOrder =
				m_orderBy->at(i).descending ? TABLE_SORT_DESCEND : TABLE_SORT_ASCEND;
		}
	}

	return result;
}

mapi_ptr<SRestriction> TableDirectives::seek() const
{
	mapi_ptr<SRestriction> result;

	if (m_seek && *m_seek)
	{
		LPSPropValue pval = nullptr;
		LPBYTE pb = nullptr;
		const ULONG cbAlloc =
			static_cast<ULONG>(sizeof(*result) + sizeof(*pval) + (*m_seek)->size());

		CORt(::MAPIAllocateBuffer(cbAlloc, reinterpret_cast<void**>(&out_ptr { result })));
		CFRt(result != nullptr);
		pval = reinterpret_cast<LPSPropValue>(result.get() + 1);
		pb = reinterpret_cast<LPBYTE>(pval + 1);

		memmove(pb, (*m_seek)->data(), (*m_seek)->size());

		pval->ulPropTag = PR_ENTRYID;
		pval->Value.bin.cb = static_cast<ULONG>((*m_seek)->size());
		pval->Value.bin.lpb = pb;

		result->rt = RES_PROPERTY;
		result->res.resProperty.relop = RELOP_EQ;
		result->res.resProperty.ulPropTag = pval->ulPropTag;
		result->res.resProperty.lpProp = pval;
	}

	return result;
}

BOOKMARK TableDirectives::seekBookmark() const
{
	return m_seek ? (*m_seek ? BOOKMARK_CURRENT : BOOKMARK_END) : BOOKMARK_BEGINNING;
}

LONG TableDirectives::offset() const
{
	return static_cast<LONG>(m_offset ? *m_offset : 0);
}

LONG TableDirectives::take() const
{
	return static_cast<LONG>(!m_take || *m_take == 0
			? 50				   // Default to 50 if 0 was specified.
			: std::max<LONG>(-50,  // Floor negative numbers at -50.
				std::min<LONG>(50, // Cap positive numbers at 50.
					*m_take)));
}

} // namespace graphql::mapi
