// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "DateTime.h"
#include "Guid.h"
#include "Types.h"

#include "FolderObject.h"
#include "PropertyObject.h"

namespace graphql::mapi {

// Comparator for mapi_ptr<MAPINAMEID> for std::map
bool CompareMAPINAMEID::operator()(
	const mapi_ptr<MAPINAMEID>& lhs, const mapi_ptr<MAPINAMEID>& rhs) const noexcept
{
	// If the lpguid propset IDs are not equal, the one with the lesser lpguid is less than the
	// other.
	const int compareGuids = memcmp(lhs->lpguid, rhs->lpguid, sizeof(*lhs->lpguid));

	if (compareGuids != 0)
	{
		return compareGuids < 0;
	}

	// If the ulKind union types are not equal, the one with the lesser ulKind is less than the
	// other.
	if (lhs->ulKind != rhs->ulKind)
	{
		return lhs->ulKind < rhs->ulKind;
	}

	// If the ulKind is MNID_ID, then the one with the lesser Kind.lID is less than the other.
	if (lhs->ulKind == MNID_ID)
	{
		return lhs->Kind.lID < rhs->Kind.lID;
	}

	// Otherwise the one with the lesser Kind.lpwstrName is less than the other.
	return wcscmp(lhs->Kind.lpwstrName, rhs->Kind.lpwstrName) < 0;
}

// Index into m_storeProps
enum class StoreProp : size_t
{
	IPMSubtree,	  // PR_IPM_SUBTREE_ENTRYID - Store Root
	DeletedItems, // PR_IPM_WASTEBASKET_ENTRYID - DELETED
	Outbox,		  // PR_IPM_OUTBOX_ENTRYID - OUTBOX
	SentItems,	  // PR_IPM_SENTMAIL_ENTRYID - SENT
};

// These properties come from the Inbox, but they fallback to the IPM Subtree.
constexpr ULONG PR_IPM_APPOINTMENT_ENTRYID = PROP_TAG(PT_BINARY, 0x36D0); // CALENDAR
constexpr ULONG PR_IPM_CONTACT_ENTRYID = PROP_TAG(PT_BINARY, 0x36D1);	  // CONTACTS
constexpr ULONG PR_IPM_TASK_ENTRYID = PROP_TAG(PT_BINARY, 0x36D4);		  // TASKS
constexpr ULONG PR_IPM_ARCHIVE_ENTRYID = PROP_TAG(PT_BINARY, 0x35FF);	  // ARCHIVE
constexpr ULONG PR_IPM_DRAFTS_ENTRYID = PROP_TAG(PT_BINARY, 0x36D7);	  // DRAFTS

// Index into m_ipmSubtreeProps and m_inboxProps
enum class FolderProp : size_t
{
	Calendar, // PR_IPM_APPOINTMENT_ENTRYID
	Contacts, // PR_IPM_CONTACT_ENTRYID
	Tasks,	  // PR_IPM_TASK_ENTRYID
	Archive,  // PR_IPM_ARCHIVE_ENTRYID
	Drafts,	  // PR_IPM_DRAFTS_ENTRYID
};

constexpr ULONG GetColumnPropType(Store::DefaultColumn column)
{
	return PROP_TYPE(Store::GetStoreColumns()[static_cast<size_t>(column)]);
}

static_assert(GetColumnPropType(Store::DefaultColumn::Id) == PT_BINARY, "type mismatch");
static_assert(GetColumnPropType(Store::DefaultColumn::Name) == PT_UNICODE, "type mismatch");

Store::Store(
	const CComPtr<IMAPISession>& session, size_t columnCount, mapi_ptr<SPropValue>&& columns)
	: m_session { session }
	, m_columnCount { columnCount }
	, m_columns { std::move(columns) }
	, m_id { GetIdColumn(DefaultColumn::Id) }
	, m_name { GetStringColumn(DefaultColumn::Name) }
{
}

Store::~Store()
{
	// Release all of the MAPI objects we opened and free any MAPI memory allocations.
	if (m_rootFolderSink)
	{
		m_rootFolderSink->Unadvise();
	}
}

const CComPtr<IMsgStore>& Store::store()
{
	OpenStore();
	return m_store;
}

const response::IdType& Store::id() const
{
	return m_id;
}

const response::IdType& Store::rootId() const
{
	return m_rootId;
}

const std::vector<std::shared_ptr<Folder>>& Store::rootFolders()
{
	LoadRootFolders({});
	return *m_rootFolders;
}

std::shared_ptr<Folder> Store::lookupRootFolder(const response::IdType& id)
{
	LoadRootFolders({});

	auto itr = m_rootFolderIds->find(id);

	if (itr == m_rootFolderIds->cend())
	{
		return nullptr;
	}

	return m_rootFolders->at(itr->second);
}

const std::map<SpecialFolder, response::IdType>& Store::specialFolders()
{
	LoadSpecialFolders();
	return *m_specialFolders;
}

std::shared_ptr<Folder> Store::lookupSpecialFolder(SpecialFolder id)
{
	LoadSpecialFolders();

	auto itr = m_specialFolders->find(id);

	if (itr == m_specialFolders->cend())
	{
		return nullptr;
	}

	return OpenFolder(itr->second);
}

std::vector<std::pair<ULONG, LPMAPINAMEID>> Store::lookupPropIdInputs(
	std::vector<PropIdInput>&& namedProps)
{
	std::vector<std::pair<ULONG, LPMAPINAMEID>> result(namedProps.size());
	std::vector<std::pair<mapi_ptr<MAPINAMEID>, size_t>> resolve;

	resolve.reserve(namedProps.size());
	for (size_t i = 0; i < namedProps.size(); ++i)
	{
		const auto& id = namedProps[i];

		if (id.id)
		{
			// Regular PROP_ID, just add it to the result list.
			const ULONG propId = static_cast<ULONG>(*id.id);

			CFRt(!id.named);
			result[i] = std::make_pair(PROP_TAG(PT_UNSPECIFIED, propId), nullptr);
			continue;
		}

		if (!id.named)
		{
			constexpr bool Empty_PropIdInput = false;
			CFRt(Empty_PropIdInput);
		}

		// It's a named prop, need to break it down further.
		mapi_ptr<MAPINAMEID> namedId;

		CORt(::MAPIAllocateBuffer(sizeof(*namedId) + sizeof(*namedId->lpguid),
			reinterpret_cast<void**>(&out_ptr { namedId })));

		const auto& propset = id.named->propset.get<std::string>();

		namedId->lpguid = reinterpret_cast<LPGUID>(namedId.get() + 1);
		*namedId->lpguid = convert::guid::from_string(propset);

		if (id.named->id)
		{
			CFRt(!id.named->name);
			namedId->ulKind = MNID_ID;
			namedId->Kind.lID = *id.named->id;
		}
		else if (id.named->name)
		{
			auto name = convert::utf8::to_utf16(*id.named->name);

			namedId->ulKind = MNID_STRING;
			CORt(::MAPIAllocateMore((name.size() + 1) * sizeof(wchar_t),
				namedId.get(),
				reinterpret_cast<void**>(&namedId->Kind.lpwstrName)));
			CFRt(namedId->Kind.lpwstrName != nullptr);
			std::copy(name.cbegin(), name.cend(), namedId->Kind.lpwstrName);
			namedId->Kind.lpwstrName[name.size()] = L'\0';
		}
		else
		{
			constexpr bool Missing_NamedId = false;
			CFRt(Missing_NamedId);
		}

		const auto itr = m_nameIdToPropIds.find(namedId);

		if (itr != m_nameIdToPropIds.cend())
		{
			// Already cached, just add it directly to the results.
			result[i] = std::make_pair(itr->second, itr->first.get());
		}
		else
		{
			// Not cached, add it to the list to resolve.
			result[i] = std::make_pair(PR_NULL, nullptr);
			resolve.push_back(std::make_pair(std::move(namedId), i));
		}
	}

	if (!resolve.empty())
	{
		std::vector<LPMAPINAMEID> pmnids;
		mapi_ptr<SPropTagArray> namedPropIds;

		pmnids.resize(resolve.size());
		std::transform(resolve.cbegin(),
			resolve.cend(),
			pmnids.begin(),
			[](const auto& entry) noexcept {
				return entry.first.get();
			});

		CORt(store()->GetIDsFromNames(static_cast<ULONG>(pmnids.size()),
			pmnids.data(),
			0,
			&out_ptr { namedPropIds }));
		CFRt(nullptr != namedPropIds);
		CFRt(static_cast<size_t>(namedPropIds->cValues) == resolve.size());
		for (size_t i = 0; i < resolve.size(); ++i)
		{
			const ULONG namedPropId = namedPropIds->aulPropTag[i];
			const size_t offset = resolve[i].second;

			if (PROP_TYPE(namedPropId) == PT_ERROR)
			{
				result[offset] = std::make_pair(PR_NULL, nullptr);
				continue;
			}

			const ULONG propId = PROP_TAG(PT_UNSPECIFIED, PROP_ID(namedPropId));
			auto itr =
				m_nameIdToPropIds.insert(std::make_pair(std::move(resolve[i].first), propId)).first;

			result[offset] = std::make_pair(propId, itr->first.get());
		}
	}

	return result;
}

std::vector<std::pair<ULONG, LPMAPINAMEID>> Store::lookupPropIds(const std::vector<ULONG>& propIds)
{
	std::vector<std::pair<ULONG, LPMAPINAMEID>> result(propIds.size());
	std::vector<std::pair<ULONG, size_t>> resolve;

	resolve.reserve(propIds.size());
	for (size_t i = 0; i < propIds.size(); ++i)
	{
		const ULONG propId = PROP_ID(propIds[i]);

		if (propId < 0x8000)
		{
			// Regular PROP_ID, just add it to the result list.
			result[i] = std::make_pair(PROP_TAG(PT_UNSPECIFIED, propId), nullptr);
			continue;
		}

		const auto itr = std::find_if(m_nameIdToPropIds.cbegin(),
			m_nameIdToPropIds.cend(),
			[propId](const auto& entry) noexcept {
				return PROP_ID(entry.second) == propId;
			});

		if (itr != m_nameIdToPropIds.cend())
		{
			// Already cached, just add it directly to the results.
			result[i] = std::make_pair(itr->second, itr->first.get());
		}
		else
		{
			// Not cached, add it to the list to resolve.
			result[i] = std::make_pair(PR_NULL, nullptr);
			resolve.push_back(std::make_pair(propId, i));
		}
	}

	if (!resolve.empty())
	{
		mapi_ptr<SPropTagArray> propIds;

		CORt(::MAPIAllocateBuffer(CbNewSPropTagArray(resolve.size()),
			reinterpret_cast<void**>(&out_ptr { propIds })));
		CFRt(propIds != nullptr);

		propIds->cValues = static_cast<ULONG>(resolve.size());
		std::transform(resolve.begin(),
			resolve.end(),
			propIds->aulPropTag,
			[](const auto& entry) noexcept -> ULONG {
				return PROP_TAG(PT_UNSPECIFIED, entry.first);
			});

		LPSPropTagArray pPropIds = propIds.get();
		ULONG cPropNames = 0;
		mapi_ptr<LPMAPINAMEID> propNames;

		CORt(store()->GetNamesFromIDs(&pPropIds, nullptr, 0, &cPropNames, &out_ptr { propNames }));
		CFRt(static_cast<size_t>(cPropNames) == resolve.size());
		CFRt(propNames != nullptr);

		for (size_t i = 0; i < resolve.size(); ++i)
		{
			const LPMAPINAMEID name = propNames.get()[i];
			const ULONG propId = PROP_TAG(PT_UNSPECIFIED, resolve[i].first);
			const size_t offset = resolve[i].second;
			mapi_ptr<MAPINAMEID> namedId;

			CORt(::MAPIAllocateBuffer(sizeof(*namedId) + sizeof(*namedId->lpguid),
				reinterpret_cast<void**>(&out_ptr { namedId })));

			namedId->lpguid = reinterpret_cast<LPGUID>(namedId.get() + 1);
			memmove(namedId->lpguid, name->lpguid, sizeof(namedId->lpguid));
			namedId->ulKind = name->ulKind;

			if (name->ulKind == MNID_STRING)
			{
				std::wstring_view source { name->Kind.lpwstrName };

				namedId->ulKind = MNID_STRING;
				CORt(::MAPIAllocateMore((source.size() + 1) * sizeof(wchar_t),
					namedId.get(),
					reinterpret_cast<void**>(&namedId->Kind.lpwstrName)));
				CFRt(namedId->Kind.lpwstrName != nullptr);
				std::copy(source.cbegin(), source.cend(), namedId->Kind.lpwstrName);
				namedId->Kind.lpwstrName[source.size()] = L'\0';
			}
			else
			{
				CFRt(name->ulKind == MNID_ID);
				namedId->Kind.lID = name->Kind.lID;
			}

			auto itr = m_nameIdToPropIds.insert(std::make_pair(std::move(namedId), propId)).first;

			result[offset] = std::make_pair(itr->second, itr->first.get());
		}
	}

	return result;
}

const SPropValue& Store::GetColumnProp(DefaultColumn column) const
{
	const auto index = static_cast<size_t>(column);

	CFRt(m_columnCount > static_cast<size_t>(index));

	return m_columns.get()[index];
}

response::IdType Store::GetIdColumn(DefaultColumn column) const
{
	const auto& idProp = GetColumnProp(column);

	if (PROP_TYPE(idProp.ulPropTag) != PT_BINARY)
	{
		return {};
	}

	const auto idBegin = reinterpret_cast<std::uint8_t*>(idProp.Value.bin.lpb);
	const auto idEnd = idBegin + idProp.Value.bin.cb;

	return { idBegin, idEnd };
}

std::string Store::GetStringColumn(DefaultColumn column) const
{
	const auto& stringProp = GetColumnProp(column);

	if (PROP_TYPE(stringProp.ulPropTag) != PT_UNICODE)
	{
		return {};
	}

	return convert::utf8::to_utf8(stringProp.Value.lpszW);
}

std::vector<std::shared_ptr<object::Property>> Store::GetColumns(
	size_t columnCount, const LPSPropValue columns)
{
	std::map<ULONG, Property::id_variant> idMap;
	std::vector<std::pair<Property::id_variant, mapi_ptr<SPropValue>>> idValuePairs;
	LPSPropValue propBegin = columns;
	LPSPropValue propEnd = columns + columnCount;
	std::vector<ULONG> propIds(columnCount);

	std::transform(propBegin, propEnd, propIds.begin(), [](const SPropValue& value) noexcept {
		return value.ulPropTag;
	});

	auto resolved = lookupPropIds(propIds);

	for (const auto& entry : resolved)
	{
		const ULONG propId = PROP_ID(entry.first);
		const LPMAPINAMEID name = entry.second;

		if (name == nullptr)
		{
			idMap[propId] = propId;
		}
		else
		{
			idMap[propId] = *name;
		}
	}

	// Double check that we mapped all of the property IDs.
	CFRt(idMap.size() == columnCount);

	// Split the values allocation into a separate buffer for each property.
	idValuePairs.reserve(columnCount);
	std::transform(propBegin,
		propEnd,
		std::back_insert_iterator(idValuePairs),
		[&idMap](SPropValue& prop) {
			mapi_ptr<SPropValue> dupe;

			CORt(ScDupPropset(1, &prop, ::MAPIAllocateBuffer, &out_ptr { dupe }));
			CFRt(dupe != nullptr);

			const ULONG propId = PROP_ID(prop.ulPropTag);

			return std::make_pair(std::move(idMap[propId]), std::move(dupe));
		});

	std::vector<std::shared_ptr<object::Property>> result(idValuePairs.size());

	std::transform(idValuePairs.begin(), idValuePairs.end(), result.begin(), [](auto& entry) {
		return std::make_shared<object::Property>(
			std::make_shared<Property>(std::move(entry.first), std::move(entry.second)));
	});

	return result;
}

std::vector<std::shared_ptr<object::Property>> Store::GetProperties(
	IMAPIProp* pObject, std::optional<std::vector<Column>>&& idsArg)
{
	std::map<ULONG, Property::id_variant> idMap;
	std::vector<std::pair<Property::id_variant, mapi_ptr<SPropValue>>> idValuePairs;
	ULONG cValues = 0;
	mapi_ptr<SPropValue> props;
	LPSPropValue propBegin = nullptr;
	LPSPropValue propEnd = nullptr;

	if (idsArg && !idsArg->empty())
	{
		// Only get selected properties.
		std::vector<PropIdInput> inputs(idsArg->size());

		std::transform(idsArg->cbegin(),
			idsArg->cend(),
			inputs.begin(),
			[](const Column& column) noexcept {
				return column.property;
			});

		auto resolved = lookupPropIdInputs(std::move(inputs));

		CFRt(resolved.size() == idsArg->size());

		mapi_ptr<SPropTagArray> propIds;

		CORt(::MAPIAllocateBuffer(CbNewSPropTagArray(resolved.size()),
			reinterpret_cast<void**>(&out_ptr { propIds })));
		CFRt(propIds != nullptr);
		propIds->cValues = static_cast<ULONG>(resolved.size());

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

			CFRt(static_cast<size_t>(idsArg->at(i).type) < c_propTypes.size());

			const auto& entry = resolved[i];
			const auto propType = c_propTypes[static_cast<size_t>(idsArg->at(i).type)];
			const ULONG propId = PROP_ID(entry.first);
			const LPMAPINAMEID name = entry.second;

			propIds->aulPropTag[i] = PROP_TAG(propType, propId);

			if (name == nullptr)
			{
				idMap[propId] = propId;
			}
			else
			{
				idMap[propId] = *name;
			}
		}

		// Get the props
		LPSPropValue values = nullptr;

		CORt(pObject->GetProps(propIds.get(), MAPI_UNICODE, &cValues, &values));
		CFRt(values != nullptr);
		CFRt(cValues > 0);
		props.reset(values);
		propBegin = props.get();
		propEnd = propBegin + static_cast<size_t>(cValues);
	}
	else
	{
		// Get all of the properties.
		LPSPropValue values = nullptr;

		CORt(pObject->GetProps(nullptr, MAPI_UNICODE, &cValues, &values));
		CFRt(values != nullptr);
		CFRt(cValues > 0);
		props.reset(values);
		propBegin = props.get();
		propEnd = propBegin + static_cast<size_t>(cValues);

		std::vector<ULONG> propIds(static_cast<size_t>(cValues));

		std::transform(propBegin, propEnd, propIds.begin(), [](const SPropValue& value) noexcept {
			return value.ulPropTag;
		});

		auto resolved = lookupPropIds(propIds);

		for (const auto& entry : resolved)
		{
			const ULONG propId = PROP_ID(entry.first);
			const LPMAPINAMEID name = entry.second;

			if (name == nullptr)
			{
				idMap[propId] = propId;
			}
			else
			{
				idMap[propId] = *name;
			}
		}
	}

	// Double check that we mapped all of the property IDs.
	CFRt(idMap.size() == static_cast<size_t>(cValues));

	// Split the values allocation into a separate buffer for each property.
	idValuePairs.reserve(static_cast<size_t>(cValues));
	std::transform(propBegin,
		propEnd,
		std::back_insert_iterator(idValuePairs),
		[&idMap](SPropValue& prop) {
			mapi_ptr<SPropValue> dupe;

			CORt(ScDupPropset(1, &prop, ::MAPIAllocateBuffer, &out_ptr { dupe }));
			CFRt(dupe != nullptr);

			const ULONG propId = PROP_ID(prop.ulPropTag);

			return std::make_pair(std::move(idMap[propId]), std::move(dupe));
		});

	std::vector<std::shared_ptr<object::Property>> result(idValuePairs.size());

	std::transform(idValuePairs.begin(), idValuePairs.end(), result.begin(), [](auto& entry) {
		return std::make_shared<object::Property>(
			std::make_shared<Property>(std::move(entry.first), std::move(entry.second)));
	});

	return result;
}

void Store::ConvertPropertyInputs(void* pAllocMore, LPSPropValue propBegin, LPSPropValue propEnd,
	std::vector<PropertyInput>&& input)
{
	std::vector<PropIdInput> propIds(input.size());

	std::transform(input.begin(), input.end(), propIds.begin(), [](auto& entry) noexcept {
		auto propId = std::move(entry.id);
		return propId;
	});

	auto resolved = lookupPropIdInputs(std::move(propIds));

	CFRt(resolved.size() == input.size());
	for (size_t i = 0; i < resolved.size(); ++i)
	{
		CFRt(propBegin < propEnd);

		const ULONG propId = PROP_ID(resolved[i].first);
		auto value = std::move(input.at(i).value);
		auto& prop = *propBegin++;

		// Each type of value should be mutually exclusive so in each case, check to make sure the
		// others are null.
		if (value.integer)
		{
			CFRt(!value.boolean);
			CFRt(!value.string);
			CFRt(!value.guid);
			CFRt(!value.time);
			CFRt(!value.bin);
			CFRt(!value.stream);

			prop.ulPropTag = PROP_TAG(PT_LONG, propId);
			prop.Value.l = static_cast<LONG>(*value.integer);
		}
		else if (value.boolean)
		{
			CFRt(!value.string);
			CFRt(!value.guid);
			CFRt(!value.time);
			CFRt(!value.bin);
			CFRt(!value.stream);

			prop.ulPropTag = PROP_TAG(PT_BOOLEAN, propId);
			prop.Value.b = *value.boolean;
		}
		else if (value.string)
		{
			CFRt(!value.guid);
			CFRt(!value.time);
			CFRt(!value.bin);
			CFRt(!value.stream);

			auto str = convert::utf8::to_utf16(*value.string);
			const size_t strSize = (str.size() + 1) * sizeof(wchar_t);

			prop.ulPropTag = PROP_TAG(PT_UNICODE, propId);
			CORt(::MAPIAllocateMore(static_cast<ULONG>(strSize),
				pAllocMore,
				reinterpret_cast<void**>(&prop.Value.lpszW)));
			CFRt(prop.Value.lpszW != nullptr);
			memmove(prop.Value.lpszW, str.data(), strSize);
			prop.Value.lpszW[str.size()] = L'\0';
		}
		else if (value.guid)
		{
			CFRt(!value.time);
			CFRt(!value.bin);
			CFRt(!value.stream);

			const auto& str = value.guid->get<std::string>();

			prop.ulPropTag = PROP_TAG(PT_CLSID, propId);
			CORt(::MAPIAllocateMore(static_cast<ULONG>(sizeof(*prop.Value.lpguid)),
				pAllocMore,
				reinterpret_cast<void**>(&prop.Value.lpguid)));
			CFRt(prop.Value.lpguid != nullptr);
			*prop.Value.lpguid = convert::guid::from_string(str);
		}
		else if (value.time)
		{
			CFRt(!value.bin);
			CFRt(!value.stream);

			const auto& str = value.guid->get<std::string>();

			prop.ulPropTag = PROP_TAG(PT_SYSTIME, propId);
			prop.Value.ft = convert::datetime::from_string(str);
		}
		else if (value.bin)
		{
			CFRt(!value.stream);

			prop.ulPropTag = PROP_TAG(PT_BINARY, propId);
			prop.Value.bin.cb = static_cast<ULONG>(value.bin->size());
			CORt(::MAPIAllocateMore(prop.Value.bin.cb,
				pAllocMore,
				reinterpret_cast<void**>(&prop.Value.bin.lpb)));
			CFRt(prop.Value.bin.lpb != nullptr);
			memmove(prop.Value.bin.lpb, value.bin->data(), value.bin->size());
		}
		else if (value.stream)
		{
			constexpr bool Stream_NotYetImplemented = false;
			CFRt(Stream_NotYetImplemented);
		}
		else
		{
			constexpr bool Missing_PropValueInput = false;
			CFRt(Missing_PropValueInput);
		}
	}

	CFRt(propBegin == propEnd);
}

std::shared_ptr<Folder> Store::OpenFolder(const response::IdType& folderId)
{
	auto itr = m_folderCache.find(folderId);

	if (itr != m_folderCache.cend())
	{
		return itr->second;
	}

	ULONG objType = 0;
	CComPtr<IMAPIFolder> folder;

	CORt(
		store()->OpenEntry(static_cast<ULONG>(folderId.empty() ? m_rootId.size() : folderId.size()),
			reinterpret_cast<LPENTRYID>(
				const_cast<response::IdType&>(folderId.empty() ? m_rootId : folderId).data()),
			&IID_IMAPIFolder,
			MAPI_BEST_ACCESS | MAPI_DEFERRED_ERRORS,
			&objType,
			reinterpret_cast<LPUNKNOWN*>(&folder)));
	CFRt(folder != nullptr);
	CFRt(objType == MAPI_FOLDER);

	auto folderProps = GetFolderProperties();
	ULONG cValues = 0;
	mapi_ptr<SPropValue> values;

	CORt(folder->GetProps(folderProps.get(), MAPI_UNICODE, &cValues, &out_ptr { values }));
	CFRt(cValues == folderProps->cValues);
	CFRt(values != nullptr);

	auto result = std::make_shared<Folder>(shared_from_this(),
		folder,
		static_cast<size_t>(cValues),
		std::move(values));

	CacheFolder(result);
	return result;
}

std::shared_ptr<Item> Store::OpenItem(const response::IdType& itemId)
{
	auto itr = m_itemCache.find(itemId);

	if (itr != m_itemCache.cend())
	{
		return itr->second;
	}

	ULONG objType = 0;
	CComPtr<IMessage> item;

	CORt(store()->OpenEntry(static_cast<ULONG>(itemId.size()),
		reinterpret_cast<LPENTRYID>(const_cast<response::IdType&>(itemId).data()),
		&IID_IMessage,
		MAPI_BEST_ACCESS | MAPI_DEFERRED_ERRORS,
		&objType,
		reinterpret_cast<LPUNKNOWN*>(&item)));
	CFRt(item != nullptr);
	CFRt(objType == MAPI_MESSAGE);

	auto itemProps = GetItemProperties();
	ULONG cValues = 0;
	mapi_ptr<SPropValue> values;

	CORt(item->GetProps(itemProps.get(), MAPI_UNICODE, &cValues, &out_ptr { values }));
	CFRt(cValues == itemProps->cValues);
	CFRt(values != nullptr);

	auto result = std::make_shared<Item>(shared_from_this(),
		item,
		static_cast<size_t>(cValues),
		std::move(values));

	CacheItem(result);
	return result;
}

void Store::CacheFolder(const std::shared_ptr<Folder>& folder)
{
	m_folderCache.insert(std::make_pair(folder->id(), folder));
}

void Store::CacheItem(const std::shared_ptr<Item>& item)
{
	m_itemCache.insert(std::make_pair(item->id(), item));
}

void Store::ClearCaches()
{
	m_folderCache.clear();
	m_itemCache.clear();
}

void Store::OpenStore()
{
	if (m_store)
	{
		return;
	}

	CORt(m_session->OpenMsgStore(NULL,
		static_cast<ULONG>(m_id.size()),
		reinterpret_cast<LPENTRYID>(const_cast<response::IdType&>(m_id).data()),
		&IID_IMsgStore,
		MAPI_BEST_ACCESS | MAPI_DEFERRED_ERRORS,
		&m_store));

	// These properties always come from the IMsgStore.
	SizedSPropTagArray(4, storeIdProps) = { 4,
		{
			PR_IPM_SUBTREE_ENTRYID,
			PR_IPM_WASTEBASKET_ENTRYID,
			PR_IPM_OUTBOX_ENTRYID,
			PR_IPM_SENTMAIL_ENTRYID,
		} };

	ULONG cValues = 0;
	LPSPropValue storeIds = nullptr;

	CORt(m_store->GetProps(reinterpret_cast<LPSPropTagArray>(&storeIdProps),
		MAPI_UNICODE,
		&cValues,
		&storeIds));
	CFRt(storeIds != nullptr);
	m_storeProps.reset(storeIds);
	CFRt(cValues == storeIdProps.cValues);

	SizedSPropTagArray(5, folderIdProps) = { 5,
		{
			PR_IPM_APPOINTMENT_ENTRYID,
			PR_IPM_CONTACT_ENTRYID,
			PR_IPM_TASK_ENTRYID,
			PR_IPM_ARCHIVE_ENTRYID,
			PR_IPM_DRAFTS_ENTRYID,
		} };

	// The store must have an IPM Subtree.
	LPSPropValue pRootId = storeIds + static_cast<size_t>(StoreProp::IPMSubtree);

	CFRt(PROP_TYPE(pRootId->ulPropTag) == PT_BINARY);

	const auto rootIdBegin = reinterpret_cast<std::uint8_t*>(pRootId->Value.bin.lpb);
	const auto rootIdEnd = rootIdBegin + static_cast<size_t>(pRootId->Value.bin.cb);

	m_rootId = { rootIdBegin, rootIdEnd };

	ULONG objType = 0;

	CORt(m_store->OpenEntry(pRootId->Value.bin.cb,
		reinterpret_cast<LPENTRYID>(pRootId->Value.bin.lpb),
		&IID_IMAPIFolder,
		MAPI_BEST_ACCESS | MAPI_DEFERRED_ERRORS,
		&objType,
		reinterpret_cast<LPUNKNOWN*>(&m_ipmSubtree)));
	CFRt(m_ipmSubtree != nullptr) CFRt(objType == MAPI_FOLDER);

	LPSPropValue folderIds = nullptr;

	CORt(m_ipmSubtree->GetProps(reinterpret_cast<LPSPropTagArray>(&folderIdProps),
		MAPI_UNICODE,
		&cValues,
		&folderIds));
	CFRt(folderIds != nullptr);
	m_ipmSubtreeProps.reset(folderIds);
	CFRt(cValues == folderIdProps.cValues);

	ULONG cbInboxId = 0;
	LPENTRYID peidInboxId = nullptr;

	// Try to open the Inbox, if this store has one.
	if (SUCCEEDED(
			m_store->GetReceiveFolder(nullptr, MAPI_UNICODE, &cbInboxId, &peidInboxId, nullptr))
		&& cbInboxId > 0 && peidInboxId != nullptr)
	{
		const auto idBegin = reinterpret_cast<std::uint8_t*>(peidInboxId);
		const auto idEnd = idBegin + static_cast<size_t>(cbInboxId);
		response::IdType id { idBegin, idEnd };
		CComPtr<IMAPIFolder> inbox;

		CORt(m_store->OpenEntry(cbInboxId,
			peidInboxId,
			&IID_IMAPIFolder,
			MAPI_BEST_ACCESS | MAPI_DEFERRED_ERRORS,
			&objType,
			reinterpret_cast<LPUNKNOWN*>(&inbox)));
		CFRt(inbox != nullptr) CFRt(objType == MAPI_FOLDER);

		m_cbInboxId = cbInboxId;
		m_eidInboxId.reset(peidInboxId);

		CORt(inbox->GetProps(reinterpret_cast<LPSPropTagArray>(&folderIdProps),
			MAPI_UNICODE,
			&cValues,
			&folderIds));
		CFRt(folderIds != nullptr);
		m_inboxProps.reset(folderIds);
		CFRt(cValues == folderIdProps.cValues);
	}
}

void Store::LoadSpecialFolders()
{
	if (m_specialFolders)
	{
		return;
	}

	m_specialFolders = std::make_unique<std::map<SpecialFolder, response::IdType>>();

	// Build the list of IDs from the properties we collected off of the store, the Inbox, and the
	// IPM Subtree.
	std::map<SpecialFolder, SBinary> idMap;

	OpenStore();
	FillInStoreProps(m_storeProps.get(), idMap);

	if (m_inboxProps)
	{
		idMap[SpecialFolder::INBOX] =
			SBinary { m_cbInboxId, reinterpret_cast<LPBYTE>(m_eidInboxId.get()) };
		FillInFolderProps(m_inboxProps.get(), idMap);
	}

	FillInFolderProps(m_ipmSubtreeProps.get(), idMap);

	// Open each of the special folders and get the properties we need from them directly.
	for (const auto& entry : idMap)
	{
		const auto specialFolder = entry.first;
		const ULONG cbEid = entry.second.cb;
		LPENTRYID pEid = reinterpret_cast<LPENTRYID>(entry.second.lpb);
		const auto idBegin = reinterpret_cast<std::uint8_t*>(pEid);
		const auto idEnd = idBegin + static_cast<size_t>(cbEid);
		response::IdType id { idBegin, idEnd };

		m_specialFolders->insert(std::make_pair(specialFolder, std::move(id)));
	}
}

void Store::LoadRootFolders(service::Directives&& fieldDirectives)
{
	if (m_rootFolderDirectives != fieldDirectives)
	{
		// Reset the subFolders and reload if the directives change
		m_rootFolders.reset();
		m_rootFolderDirectives = std::move(fieldDirectives);
	}

	if (m_rootFolders)
	{
		return;
	}

	m_rootFolderIds = std::make_unique<std::map<response::IdType, size_t>>();
	m_rootFolders = std::make_unique<std::vector<std::shared_ptr<Folder>>>();

	auto folderProps = GetFolderProperties();
	constexpr auto c_folderSorts = Folder::GetFolderSorts();
	mapi_ptr<SSortOrderSet> folderSorts;

	CORt(::MAPIAllocateBuffer(CbNewSSortOrderSet(c_folderSorts.size()),
		reinterpret_cast<void**>(&out_ptr { folderSorts })));
	CFRt(folderSorts != nullptr);
	folderSorts->cSorts = static_cast<ULONG>(c_folderSorts.size());
	folderSorts->cCategories = 0;
	folderSorts->cExpanded = 0;
	std::copy(c_folderSorts.begin(), c_folderSorts.end(), folderSorts->aSort);

	OpenStore();
	LoadSpecialFolders();

	const TableDirectives directives { shared_from_this(), m_rootFolderDirectives };
	CComPtr<IMAPITable> sptable;

	CORt(m_ipmSubtree->GetHierarchyTable(MAPI_DEFERRED_ERRORS | MAPI_UNICODE, &sptable));

	const rowset_ptr sprows =
		directives.read(sptable, std::move(folderProps), std::move(folderSorts));

	m_rootFolders->reserve(static_cast<size_t>(sprows->cRows));
	for (ULONG i = 0; i != sprows->cRows; i++)
	{
		auto& row = sprows->aRow[i];
		const size_t columnCount = static_cast<size_t>(row.cValues);
		mapi_ptr<SPropValue> columns { row.lpProps };

		row.lpProps = nullptr;

		auto folder =
			std::make_shared<Folder>(shared_from_this(), nullptr, columnCount, std::move(columns));

		CacheFolder(folder);
		m_rootFolderIds->insert(std::make_pair(folder->id(), m_rootFolders->size()));
		m_rootFolders->push_back(std::move(folder));
	}

	if (!m_rootFolderSink)
	{
		auto spThis = shared_from_this();
		CComPtr<AdviseSinkProxy<IMAPITable>> sinkProxy;
		ULONG_PTR connectionId = 0;

		sinkProxy.Attach(new AdviseSinkProxy<IMAPITable>(
			[wpStore = std::weak_ptr { spThis }](size_t, LPNOTIFICATION) {
				auto spStore = wpStore.lock();

				if (spStore)
				{
					spStore->m_rootFolders.reset();
				}
			}));

		CORt(sptable->Advise(fnevTableModified, sinkProxy, &connectionId));
		sinkProxy->OnAdvise(sptable, connectionId);

		m_rootFolderSink = sinkProxy;
	}
}

mapi_ptr<SPropTagArray> Store::GetFolderProperties() const
{
	constexpr auto c_folderProps = Folder::GetFolderColumns();
	mapi_ptr<SPropTagArray> folderProps;

	CORt(::MAPIAllocateBuffer(CbNewSPropTagArray(c_folderProps.size()),
		reinterpret_cast<void**>(&out_ptr { folderProps })));
	CFRt(folderProps != nullptr);
	folderProps->cValues = static_cast<ULONG>(c_folderProps.size());
	std::copy(c_folderProps.begin(), c_folderProps.end(), folderProps->aulPropTag);

	return folderProps;
}

mapi_ptr<SPropTagArray> Store::GetItemProperties() const
{
	constexpr auto c_itemProps = Item::GetItemColumns();
	mapi_ptr<SPropTagArray> itemProps;

	CORt(::MAPIAllocateBuffer(CbNewSPropTagArray(c_itemProps.size()),
		reinterpret_cast<void**>(&out_ptr { itemProps })));
	CFRt(itemProps != nullptr);
	itemProps->cValues = static_cast<ULONG>(c_itemProps.size());
	std::copy(c_itemProps.begin(), c_itemProps.end(), itemProps->aulPropTag);

	return itemProps;
}

const response::IdType& Store::getId() const
{
	return m_id;
}

const std::string& Store::getName() const
{
	return m_name;
}

std::vector<std::shared_ptr<object::Property>> Store::getColumns()
{
	const auto offset = static_cast<size_t>(DefaultColumn::Count);

	CFRt(m_columnCount >= offset);
	return { GetColumns(m_columnCount - offset, m_columns.get() + offset) };
}

std::vector<std::shared_ptr<object::Folder>> Store::getRootFolders(
	service::FieldParams&& params, std::optional<std::vector<response::IdType>>&& idsArg)
{
	std::vector<std::shared_ptr<object::Folder>> result {};

	LoadRootFolders(std::move(params.fieldDirectives));

	if (idsArg)
	{
		// Lookup the root folders with the specified IDs
		result.resize(idsArg->size());
		std::transform(idsArg->cbegin(),
			idsArg->cend(),
			result.begin(),
			[this](const response::IdType& id) noexcept {
				return std::make_shared<object::Folder>(lookupRootFolder(id));
			});
	}
	else
	{
		// Just clone the entire vector of stores.
		result.resize(m_rootFolders->size());
		std::transform(m_rootFolders->cbegin(),
			m_rootFolders->cend(),
			result.begin(),
			[this](const std::shared_ptr<Folder>& folder) noexcept {
				return std::make_shared<object::Folder>(folder);
			});
	}

	return result;
}

std::vector<std::shared_ptr<object::Folder>> Store::getSpecialFolders(
	std::vector<SpecialFolder>&& idsArg)
{
	std::vector<std::shared_ptr<object::Folder>> result {};

	LoadSpecialFolders();

	// Lookup the special folders with the specified IDs
	result.resize(idsArg.size());
	std::transform(idsArg.cbegin(),
		idsArg.cend(),
		result.begin(),
		[this](SpecialFolder id) noexcept {
			return std::make_shared<object::Folder>(lookupSpecialFolder(id));
		});

	return result;
}

std::vector<std::shared_ptr<object::Property>> Store::getFolderProperties(
	response::IdType&& folderIdArg, std::optional<std::vector<Column>>&& idsArg)
{
	auto folder = OpenFolder(folderIdArg);

	CFRt(folder != nullptr);
	return { GetProperties(static_cast<IMAPIFolder*>(folder->folder()), std::move(idsArg)) };
}

std::vector<std::shared_ptr<object::Property>> Store::getItemProperties(
	response::IdType&& itemIdArg, std::optional<std::vector<Column>>&& idsArg)
{
	auto item = OpenItem(itemIdArg);

	CFRt(item != nullptr);
	return { GetProperties(static_cast<IMessage*>(item->message()), std::move(idsArg)) };
}

void Store::FillInStoreProps(LPSPropValue storeIds, std::map<SpecialFolder, SBinary>& idMap)
{
	// Skip IPMSubtree, but verify we got it.
	const SPropValue& ipmSubtree = storeIds[static_cast<size_t>(StoreProp::IPMSubtree)];
	CFRt(ipmSubtree.ulPropTag == PR_IPM_SUBTREE_ENTRYID);

	const std::array c_entries {
		std::make_pair(SpecialFolder::DELETED, StoreProp::DeletedItems),
		std::make_pair(SpecialFolder::OUTBOX, StoreProp::Outbox),
		std::make_pair(SpecialFolder::SENT, StoreProp::SentItems),
	};

	for (const auto& entry : c_entries)
	{
		const auto& prop = storeIds[static_cast<size_t>(entry.second)];

		if (PROP_TYPE(prop.ulPropTag) == PT_BINARY && idMap.find(entry.first) == idMap.cend())
		{
			idMap[entry.first] = prop.Value.bin;
		}
	}
}

void Store::FillInFolderProps(LPSPropValue folderIds, std::map<SpecialFolder, SBinary>& idMap)
{
	const std::array c_entries {
		std::make_pair(SpecialFolder::CALENDAR, FolderProp::Calendar),
		std::make_pair(SpecialFolder::CONTACTS, FolderProp::Contacts),
		std::make_pair(SpecialFolder::TASKS, FolderProp::Tasks),
		std::make_pair(SpecialFolder::ARCHIVE, FolderProp::Archive),
		std::make_pair(SpecialFolder::DRAFTS, FolderProp::Drafts),
	};

	for (const auto& entry : c_entries)
	{
		const auto& prop = folderIds[static_cast<size_t>(entry.second)];

		if (PROP_TYPE(prop.ulPropTag) == PT_BINARY && idMap.find(entry.first) == idMap.cend())
		{
			idMap[entry.first] = prop.Value.bin;
		}
	}
}

} // namespace graphql::mapi