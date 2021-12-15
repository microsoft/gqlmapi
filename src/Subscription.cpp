// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Guid.h"
#include "Types.h"

#include "FolderAddedObject.h"
#include "FolderChangeObject.h"
#include "FolderRemovedObject.h"
#include "FolderUpdatedObject.h"
#include "FoldersReloadedObject.h"
#include "ItemAddedObject.h"
#include "ItemChangeObject.h"
#include "ItemRemovedObject.h"
#include "ItemUpdatedObject.h"
#include "ItemsReloadedObject.h"
#include "SubscriptionObject.h"

using namespace std::literals;

namespace graphql::mapi {

Subscription::Subscription(const std::shared_ptr<Query>& query)
	: m_query { query }
{
}

Subscription::~Subscription()
{
	// Release all of the MAPI objects we opened and free any MAPI memory allocations.
	m_itemSinks.clear();
	m_subFolderSinks.clear();
	m_rootFolderSinks.clear();
}

void Subscription::setService(const std::shared_ptr<Operations>& service) noexcept
{
	m_service = service;
}

std::vector<std::shared_ptr<object::ItemChange>> Subscription::getItems(
	service::FieldParams&& params, ObjectId&& folderIdArg)
{
	Registration<Item> registration { { std::move(folderIdArg),
		std::move(params.fieldDirectives) } };
	const auto [itr, itrEnd] = m_itemSinks.equal_range(registration);

	switch (params.resolverContext)
	{
		case service::ResolverContext::NotifySubscribe:
		{
			if (itr == itrEnd)
			{
				RegisterAdviseSinkProxy<Item, ObjectId, ItemsSubscription>(params.launch,
					"items"s,
					"folderId"sv,
					registration.key.objectId,
					registration);
			}
			else
			{
				registration.sink = itr->sink;
			}

			m_itemSinks.insert(itrEnd, std::move(registration));
			break;
		}

		case service::ResolverContext::NotifyUnsubscribe:
		{
			CFRt(itr != itrEnd);
			m_itemSinks.erase(itr);
			break;
		}

		default:
		{
			constexpr bool Unexpected_ResolverContext = false;
			CFRt(Unexpected_ResolverContext);
		}
	}

	return {};
}

std::vector<std::shared_ptr<object::FolderChange>> Subscription::getSubFolders(
	service::FieldParams&& params, ObjectId&& parentFolderIdArg)
{
	Registration<Folder> registration { { std::move(parentFolderIdArg),
		std::move(params.fieldDirectives) } };
	const auto [itr, itrEnd] = m_subFolderSinks.equal_range(registration);

	switch (params.resolverContext)
	{
		case service::ResolverContext::NotifySubscribe:
		{
			if (itr == itrEnd)
			{
				RegisterAdviseSinkProxy<Folder, ObjectId, SubFoldersSubscription>(params.launch,
					"subFolders"s,
					"parentFolderId"sv,
					registration.key.objectId,
					registration);
			}
			else
			{
				registration.sink = itr->sink;
			}

			m_subFolderSinks.insert(itrEnd, std::move(registration));
			break;
		}

		case service::ResolverContext::NotifyUnsubscribe:
		{
			CFRt(itr != itrEnd);
			m_subFolderSinks.erase(itr);
			break;
		}

		default:
		{
			constexpr bool Unexpected_ResolverContext = false;
			CFRt(Unexpected_ResolverContext);
		}
	}

	return {};
}

std::vector<std::shared_ptr<object::FolderChange>> Subscription::getRootFolders(
	service::FieldParams&& params, response::IdType&& storeIdArg)
{
	Registration<Folder> registration { { { std::move(storeIdArg), {} },
		std::move(params.fieldDirectives) } };
	const auto [itr, itrEnd] = m_rootFolderSinks.equal_range(registration);

	switch (params.resolverContext)
	{
		case service::ResolverContext::NotifySubscribe:
		{
			if (itr == itrEnd)
			{
				RegisterAdviseSinkProxy<Folder, response::IdType, RootFoldersSubscription>(
					params.launch,
					"rootFolders"s,
					"storeId"sv,
					registration.key.objectId.storeId,
					registration);
			}
			else
			{
				registration.sink = itr->sink;
			}

			m_rootFolderSinks.insert(itrEnd, std::move(registration));
			break;
		}

		case service::ResolverContext::NotifyUnsubscribe:
		{
			CFRt(itr != itrEnd);
			m_rootFolderSinks.erase(itr);
			break;
		}

		default:
		{
			constexpr bool Unexpected_ResolverContext = false;
			CFRt(Unexpected_ResolverContext);
		}
	}

	return {};
}

bool operator==(const ObjectId& lhs, const ObjectId& rhs) noexcept
{
	return lhs.storeId == rhs.storeId && lhs.objectId == rhs.objectId;
}

template <class T>
struct SubscriptionTraits;

template <>
struct SubscriptionTraits<Item>
{
	using Change = object::ItemChange;

	using Added = ItemAdded;
	using Updated = ItemUpdated;
	using Removed = ItemRemoved;
	using Reloaded = ItemsReloaded;

	using AddedObject = object::ItemAdded;
	using UpdatedObject = object::ItemUpdated;
	using RemovedObject = object::ItemRemoved;
	using ReloadedObject = object::ItemsReloaded;
};

template <>
struct SubscriptionTraits<Folder>
{
	using Change = object::FolderChange;

	using Added = FolderAdded;
	using Updated = FolderUpdated;
	using Removed = FolderRemoved;
	using Reloaded = FoldersReloaded;

	using AddedObject = object::FolderAdded;
	using UpdatedObject = object::FolderUpdated;
	using RemovedObject = object::FolderRemoved;
	using ReloadedObject = object::FoldersReloaded;
};

template <class T, class ArgumentType, class PayloadType>
void Subscription::RegisterAdviseSinkProxy(service::await_async launch, std::string&& fieldName,
	std::string_view argumentName, const ArgumentType& argumentValue,
	Registration<T>& registration) const
{
	registration.sink = std::make_shared<TableSink<T>>();
	registration.sink->rows =
		LoadRows<T>(registration.key, registration.sink->store, registration.sink->table);

	auto spThis = shared_from_this();
	CComPtr<AdviseSinkProxy<IMAPITable>> sinkProxy;
	ULONG_PTR connectionId = 0;

	sinkProxy.Attach(new AdviseSinkProxy<IMAPITable>([launch,
														 fieldName = std::move(fieldName),
														 argumentName,
														 argumentValue = argumentValue,
														 key = registration.key,
														 wpThis = std::weak_ptr { spThis },
														 wpSink =
															 std::weak_ptr { registration.sink }](
														 size_t count,
														 LPNOTIFICATION pNotifications) {
		if (0 == count || nullptr == pNotifications)
		{
			return;
		}

		auto spThis = wpThis.lock();
		auto spSink = wpSink.lock();

		if (!spThis || !spSink)
		{
			return;
		}

		auto spService = spThis->m_service.lock();

		if (!spService)
		{
			return;
		}

		std::vector<std::shared_ptr<typename SubscriptionTraits<T>::Change>> items;

		items.reserve(count);
		for (size_t i = 0; i < count; ++i)
		{
			const auto& notif = pNotifications[i];
			bool reload = false;

			switch (notif.info.tab.ulTableEvent)
			{
				case TABLE_CHANGED:
				case TABLE_ERROR:
				case TABLE_RELOAD:
					reload = true;
					break;

				case TABLE_ROW_ADDED:
					if (notif.info.tab.propPrior.ulPropTag == PR_INSTANCE_KEY)
					{
						// Find the insertion point if it's in our cache window.
						const auto beginKey = reinterpret_cast<const std::uint8_t*>(
							notif.info.tab.propPrior.Value.bin.lpb);
						const auto endKey =
							beginKey + static_cast<size_t>(notif.info.tab.propPrior.Value.bin.cb);
						const response::IdType priorKey { beginKey, endKey };
						const auto itrPrior = std::find_if(spSink->rows.cbegin(),
							spSink->rows.cend(),
							[&priorKey](const std::shared_ptr<T>& item) noexcept {
								return item->instanceKey() == priorKey;
							});

						if (itrPrior == spSink->rows.cend())
						{
							break;
						}

						const auto& row = notif.info.tab.row;
						const size_t columnCount = static_cast<size_t>(row.cValues);
						mapi_ptr<SPropValue> columns;

						CORt(ScDupPropset(row.cValues,
							row.lpProps,
							::MAPIAllocateBuffer,
							&out_ptr { columns }));
						CFRt(columns != nullptr);

						const auto itr = itrPrior + 1;
						const auto index =
							static_cast<int>(std::distance(spSink->rows.cbegin(), itr));
						auto item = std::make_shared<T>(spSink->store,
							nullptr,
							columnCount,
							std::move(columns));

						spSink->rows.insert(itr, item);
						items.push_back(std::make_shared<typename SubscriptionTraits<T>::Change>(
							std::make_shared<typename SubscriptionTraits<T>::AddedObject>(
								std::make_shared<typename SubscriptionTraits<T>::Added>(index,
									std::move(item)))));
					}

					break;

				case TABLE_ROW_MODIFIED:
					if (notif.info.tab.propIndex.ulPropTag == PR_INSTANCE_KEY)
					{
						// Find the insertion point if it's in our cache window.
						const auto beginKey = reinterpret_cast<const std::uint8_t*>(
							notif.info.tab.propIndex.Value.bin.lpb);
						const auto endKey =
							beginKey + static_cast<size_t>(notif.info.tab.propIndex.Value.bin.cb);
						const response::IdType indexKey { beginKey, endKey };
						const auto itr = std::find_if(spSink->rows.begin(),
							spSink->rows.end(),
							[&indexKey](const std::shared_ptr<T>& item) noexcept {
								return item->instanceKey() == indexKey;
							});

						if (itr == spSink->rows.cend())
						{
							break;
						}

						const auto& row = notif.info.tab.row;
						const size_t columnCount = static_cast<size_t>(row.cValues);
						mapi_ptr<SPropValue> columns;

						CORt(ScDupPropset(row.cValues,
							row.lpProps,
							::MAPIAllocateBuffer,
							&out_ptr { columns }));
						CFRt(columns != nullptr);

						const auto index =
							static_cast<int>(std::distance(spSink->rows.begin(), itr));
						auto item = std::make_shared<T>(spSink->store,
							nullptr,
							columnCount,
							std::move(columns));

						*itr = item;
						items.push_back(std::make_shared<typename SubscriptionTraits<T>::Change>(
							std::make_shared<typename SubscriptionTraits<T>::UpdatedObject>(
								std::make_shared<typename SubscriptionTraits<T>::Updated>(index,
									std::move(item)))));
					}

					break;

				case TABLE_ROW_DELETED:
					if (notif.info.tab.propIndex.ulPropTag == PR_INSTANCE_KEY)
					{
						// Find the insertion point if it's in our cache window.
						const auto beginKey = reinterpret_cast<const std::uint8_t*>(
							notif.info.tab.propIndex.Value.bin.lpb);
						const auto endKey =
							beginKey + static_cast<size_t>(notif.info.tab.propIndex.Value.bin.cb);
						const response::IdType indexKey { beginKey, endKey };
						const auto itr = std::find_if(spSink->rows.cbegin(),
							spSink->rows.cend(),
							[&indexKey](const std::shared_ptr<T>& item) noexcept {
								return item->instanceKey() == indexKey;
							});

						if (itr == spSink->rows.cend())
						{
							break;
						}

						const auto index =
							static_cast<int>(std::distance(spSink->rows.cbegin(), itr));

						spSink->rows.erase(itr);
						items.push_back(std::make_shared<typename SubscriptionTraits<T>::Change>(
							std::make_shared<typename SubscriptionTraits<T>::RemovedObject>(
								std::make_shared<typename SubscriptionTraits<T>::Removed>(index,
									indexKey))));
					}

					break;
			}

			if (reload)
			{
				items.clear();
				spSink->rows = spThis->LoadRows<T>(key, spSink->store, spSink->table);
				items.push_back(std::make_shared<typename SubscriptionTraits<T>::Change>(
					std::make_shared<typename SubscriptionTraits<T>::ReloadedObject>(
						std::make_shared<typename SubscriptionTraits<T>::Reloaded>(spSink->rows))));
				break;
			}
		}

		if (!items.empty())
		{
			service::SubscriptionArgumentFilterCallback argumentsMatch =
				[argumentName, &argumentValue, &key](
					response::MapType::const_reference required) noexcept -> bool {
				if (required.first != argumentName)
				{
					return false;
				}

				const auto matchValue =
					service::ModifiedArgument<ArgumentType>::convert(required.second);

				return matchValue == argumentValue;
			};

			service::SubscriptionDirectiveFilterCallback directivesMatch =
				[&key](service::Directives::const_reference required) noexcept -> bool {
				const auto itrDirective = std::find_if(key.directives.cbegin(),
					key.directives.cend(),
					[directiveName = required.first](const auto& directive) noexcept {
						return directiveName == directive.first;
					});

				return (itrDirective != key.directives.end()
					&& itrDirective->second == required.second);
			};

			spService->deliver(
				{ { service::SubscriptionFilter { fieldName, argumentsMatch, directivesMatch } },
					launch,
					std::make_shared<object::Subscription>(
						std::make_shared<PayloadType>(std::move(items))) });
		}
	}));

	CORt(registration.sink->table->Advise(fnevTableModified, sinkProxy, &connectionId));
	sinkProxy->OnAdvise(registration.sink->table, connectionId);
	registration.sink->sinkProxy.Attach(sinkProxy.Detach());
}

template <>
std::vector<std::shared_ptr<Item>> Subscription::LoadRows<Item>(
	const RegistrationKey& key, std::shared_ptr<Store>& store, CComPtr<IMAPITable>& sptable) const
{
	if (!store)
	{
		store = m_query->lookup(key.objectId.storeId);
		CFRt(store != nullptr);
	}

	if (sptable == nullptr)
	{
		auto folder = store->OpenFolder(key.objectId.objectId);

		CFRt(folder != nullptr);
		CORt(folder->folder()->GetContentsTable(MAPI_DEFERRED_ERRORS | MAPI_UNICODE, &sptable));
	}

	constexpr auto c_itemProps = Item::GetItemColumns();
	mapi_ptr<SPropTagArray> itemProps;

	CORt(::MAPIAllocateBuffer(CbNewSPropTagArray(c_itemProps.size()),
		reinterpret_cast<void**>(&out_ptr { itemProps })));
	CFRt(itemProps != nullptr);
	itemProps->cValues = static_cast<ULONG>(c_itemProps.size());
	std::copy(c_itemProps.begin(), c_itemProps.end(), itemProps->aulPropTag);

	constexpr auto c_itemSorts = Item::GetItemSorts();
	mapi_ptr<SSortOrderSet> itemSorts;

	CORt(::MAPIAllocateBuffer(CbNewSSortOrderSet(c_itemSorts.size()),
		reinterpret_cast<void**>(&out_ptr { itemSorts })));
	CFRt(itemSorts != nullptr);
	itemSorts->cSorts = static_cast<ULONG>(c_itemSorts.size());
	itemSorts->cCategories = 0;
	itemSorts->cExpanded = 0;
	std::copy(c_itemSorts.begin(), c_itemSorts.end(), itemSorts->aSort);

	const TableDirectives directives { store, key.directives };
	const rowset_ptr sprows = directives.read(sptable, std::move(itemProps), std::move(itemSorts));
	std::vector<std::shared_ptr<Item>> items;

	items.reserve(static_cast<size_t>(sprows->cRows));
	for (ULONG i = 0; i != sprows->cRows; i++)
	{
		auto& row = sprows->aRow[i];
		const size_t columnCount = static_cast<size_t>(row.cValues);
		mapi_ptr<SPropValue> columns { row.lpProps };

		row.lpProps = nullptr;

		auto item = std::make_shared<Item>(store, nullptr, columnCount, std::move(columns));

		items.push_back(std::move(item));
	}

	return items;
}

template <>
std::vector<std::shared_ptr<Folder>> Subscription::LoadRows<Folder>(
	const RegistrationKey& key, std::shared_ptr<Store>& store, CComPtr<IMAPITable>& sptable) const
{
	if (!store)
	{
		store = m_query->lookup(key.objectId.storeId);
		CFRt(store != nullptr);
	}

	if (sptable == nullptr)
	{
		auto parentFolder = store->OpenFolder(key.objectId.objectId);

		CFRt(parentFolder != nullptr);
		CORt(parentFolder->folder()->GetHierarchyTable(MAPI_DEFERRED_ERRORS | MAPI_UNICODE,
			&sptable));
	}

	constexpr auto c_folderProps = Folder::GetFolderColumns();
	mapi_ptr<SPropTagArray> folderProps;

	CORt(::MAPIAllocateBuffer(CbNewSPropTagArray(c_folderProps.size()),
		reinterpret_cast<void**>(&out_ptr { folderProps })));
	CFRt(folderProps != nullptr);
	folderProps->cValues = static_cast<ULONG>(c_folderProps.size());
	std::copy(c_folderProps.begin(), c_folderProps.end(), folderProps->aulPropTag);

	constexpr auto c_folderSorts = Folder::GetFolderSorts();
	mapi_ptr<SSortOrderSet> folderSorts;

	CORt(::MAPIAllocateBuffer(CbNewSSortOrderSet(c_folderSorts.size()),
		reinterpret_cast<void**>(&out_ptr { folderSorts })));
	CFRt(folderSorts != nullptr);
	folderSorts->cSorts = static_cast<ULONG>(c_folderSorts.size());
	folderSorts->cCategories = 0;
	folderSorts->cExpanded = 0;
	std::copy(c_folderSorts.begin(), c_folderSorts.end(), folderSorts->aSort);

	const TableDirectives directives { store, key.directives };
	const rowset_ptr sprows =
		directives.read(sptable, std::move(folderProps), std::move(folderSorts));
	std::vector<std::shared_ptr<Folder>> folders;

	folders.reserve(static_cast<size_t>(sprows->cRows));
	for (ULONG i = 0; i != sprows->cRows; i++)
	{
		auto& row = sprows->aRow[i];
		const size_t columnCount = static_cast<size_t>(row.cValues);
		mapi_ptr<SPropValue> columns { row.lpProps };

		row.lpProps = nullptr;

		auto folder = std::make_shared<Folder>(store, nullptr, columnCount, std::move(columns));

		folders.push_back(std::move(folder));
	}

	return folders;
}

bool operator<(const ObjectId& lhs, const ObjectId& rhs) noexcept
{
	if (lhs.storeId < rhs.storeId)
	{
		return true;
	}
	else if (rhs.storeId < lhs.storeId)
	{
		return false;
	}

	return (lhs.objectId < rhs.objectId);
}

bool operator<(const PropIdInput& lhs, const PropIdInput& rhs)
{
	if (lhs.id || rhs.id)
	{
		if (!lhs.id)
		{
			CFRt(lhs.named);
			CFRt(rhs.id && !rhs.named);
			return true;
		}
		else if (!rhs.id)
		{
			CFRt(!lhs.named);
			CFRt(!rhs.id && rhs.named);
			return false;
		}

		return *lhs.id < *rhs.id;
	}

	CFRt(lhs.named && rhs.named);
	CFRt(lhs.named->propset.type() == response::Type::String);
	CFRt(rhs.named->propset.type() == response::Type::String);

	const auto lhsPropset = convert::guid::from_string(lhs.named->propset.get<std::string>());
	const auto rhsPropset = convert::guid::from_string(rhs.named->propset.get<std::string>());
	const auto comparePropset = memcmp(&lhsPropset, &rhsPropset, sizeof(lhsPropset));

	if (comparePropset < 0)
	{
		return true;
	}
	else if (comparePropset > 0)
	{
		return false;
	}

	if (lhs.named->id || rhs.named->id)
	{
		if (!lhs.named->id)
		{
			CFRt(lhs.named->name);
			CFRt(rhs.named->id && !rhs.named->name);
			return true;
		}
		else if (!rhs.named->id)
		{
			CFRt(!lhs.named->name);
			CFRt(!rhs.named->id && rhs.named->name);
			return false;
		}

		return *lhs.named->id < *rhs.named->id;
	}

	CFRt(lhs.named->name && rhs.named->name);

	return *lhs.named->name < *rhs.named->name;
}

bool operator<(const Column& lhs, const Column& rhs)
{
	if (lhs.type < rhs.type)
	{
		return true;
	}
	else if (rhs.type < lhs.type)
	{
		return false;
	}

	return lhs.property < rhs.property;
}

bool operator<(const Order& lhs, const Order& rhs)
{
	if (lhs.descending != rhs.descending)
	{
		return lhs.descending;
	}

	if (lhs.type < rhs.type)
	{
		return true;
	}
	else if (rhs.type < lhs.type)
	{
		return false;
	}

	return lhs.property < rhs.property;
}

template <typename T, service::TypeModifier... Modifiers>
int CompareDirectives(std::string_view directiveName, std::string_view argumentName,
	const service::Directives& lhs, const service::Directives& rhs)
{
	const auto itrLhs =
		std::find_if(lhs.begin(), lhs.end(), [directiveName](const auto& entry) noexcept {
			return entry.first == directiveName;
		});

	const auto itrRhs =
		std::find_if(rhs.begin(), rhs.end(), [directiveName](const auto& entry) noexcept {
			return entry.first == directiveName;
		});

	if (itrLhs != lhs.end() || itrRhs != rhs.end())
	{
		if (itrLhs == lhs.end())
		{
			return -1;
		}
		else if (itrRhs == rhs.end())
		{
			return 1;
		}

		auto lhsArgument =
			service::ModifiedArgument<T>::template find<Modifiers...>(argumentName, itrLhs->second);
		auto rhsArgument =
			service::ModifiedArgument<T>::template find<Modifiers...>(argumentName, itrRhs->second);

		if (lhsArgument.second || rhsArgument.second)
		{
			if (!lhsArgument.second)
			{
				return -1;
			}
			else if (!rhsArgument.second)
			{
				return 1;
			}

			const auto& lhsValue = lhsArgument.first;
			const auto& rhsValue = rhsArgument.first;

			if (lhsValue < rhsValue)
			{
				return -1;
			}
			else if (rhsValue < lhsValue)
			{
				return 1;
			}
		}
	}

	return 0;
}

bool Subscription::RegistrationKey::operator<(const RegistrationKey& rhs) const noexcept
{
	if (objectId < rhs.objectId)
	{
		return true;
	}
	else if (rhs.objectId < objectId)
	{
		return false;
	}

	const int compareOffset =
		CompareDirectives<int>("offset"sv, "count"sv, directives, rhs.directives);

	if (compareOffset != 0)
	{
		return compareOffset < 0;
	}

	const int compareTake = CompareDirectives<int>("take"sv, "count"sv, directives, rhs.directives);

	if (compareTake != 0)
	{
		return compareTake < 0;
	}

	const int compareSeek =
		CompareDirectives<response::IdType>("seek"sv, "id"sv, directives, rhs.directives);

	if (compareSeek != 0)
	{
		return compareSeek < 0;
	}

	const int compareColumns = CompareDirectives<Column, service::TypeModifier::List>("columns"sv,
		"ids"sv,
		directives,
		rhs.directives);

	if (compareColumns != 0)
	{
		return compareColumns < 0;
	}

	const int compareOrders = CompareDirectives<Order, service::TypeModifier::List>("orderBy"sv,
		"sorts"sv,
		directives,
		rhs.directives);

	if (compareOrders != 0)
	{
		return compareOrders < 0;
	}

	return false;
}

} // namespace graphql::mapi