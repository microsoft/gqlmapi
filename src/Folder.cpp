// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Guid.h"
#include "Types.h"

#include "FolderObject.h"
#include "ItemObject.h"
#include "StoreObject.h"

namespace graphql::mapi {

constexpr ULONG GetColumnPropType(Folder::DefaultColumn column)
{
	return PROP_TYPE(Folder::GetFolderColumns()[static_cast<size_t>(column)]);
}

static_assert(GetColumnPropType(Folder::DefaultColumn::InstanceKey) == PT_BINARY, "type mismatch");
static_assert(GetColumnPropType(Folder::DefaultColumn::Id) == PT_BINARY, "type mismatch");
static_assert(GetColumnPropType(Folder::DefaultColumn::ParentId) == PT_BINARY, "type mismatch");
static_assert(GetColumnPropType(Folder::DefaultColumn::Name) == PT_UNICODE, "type mismatch");
static_assert(GetColumnPropType(Folder::DefaultColumn::Total) == PT_LONG, "type mismatch");
static_assert(GetColumnPropType(Folder::DefaultColumn::Unread) == PT_LONG, "type mismatch");

Folder::Folder(const std::shared_ptr<Store>& store, IMAPIFolder* pFolder, size_t columnCount,
	mapi_ptr<SPropValue>&& columns)
	: m_store { store }
	, m_columnCount { columnCount }
	, m_columns { std::move(columns) }
	, m_instanceKey { GetIdColumn(DefaultColumn::InstanceKey) }
	, m_id { GetIdColumn(DefaultColumn::Id) }
	, m_parentId { GetIdColumn(DefaultColumn::ParentId) }
	, m_name { GetStringColumn(DefaultColumn::Name) }
	, m_count { GetIntColumn(DefaultColumn::Total) }
	, m_unread { GetIntColumn(DefaultColumn::Unread) }
	, m_specialFolder { [this]() {
		// Check if this folder matches a special folder, and re-use the special folder if it does.
		auto spStore = m_store.lock();
		const auto& specialFolders = spStore->specialFolders();
		auto itr = std::find_if(specialFolders.begin(),
			specialFolders.end(),
			[this, &spStore](const auto& entry) noexcept {
				ULONG result = 0;

				return SUCCEEDED(spStore->store()->CompareEntryIDs(static_cast<ULONG>(id().size()),
						   reinterpret_cast<LPENTRYID>(const_cast<response::IdType&>(id()).data()),
						   static_cast<ULONG>(entry.second.size()),
						   reinterpret_cast<LPENTRYID>(
							   const_cast<response::IdType&>(entry.second).data()),
						   0,
						   &result))
					&& result != 0;
			});

		return itr == specialFolders.end() ? std::nullopt : std::make_optional(itr->first);
	}() }
	, m_folder { pFolder }
{
}

Folder::~Folder()
{
	// Release all of the MAPI objects we opened and free any MAPI memory allocations.
	if (m_subFolderSink)
	{
		m_subFolderSink->Unadvise();
	}

	if (m_itemSink)
	{
		m_itemSink.Release();
	}
}

const response::IdType& Folder::instanceKey() const
{
	return m_instanceKey;
}

const response::IdType& Folder::id() const
{
	return m_id;
}

const std::string& Folder::name() const
{
	return m_name;
}

int Folder::count() const
{
	return m_count;
}

int Folder::unread() const
{
	return m_unread;
}

const CComPtr<IMAPIFolder>& Folder::folder()
{
	OpenFolder();
	return m_folder;
}

const std::vector<std::shared_ptr<Folder>>& Folder::subFolders()
{
	LoadSubFolders({});
	return *m_subFolders;
}

std::shared_ptr<Folder> Folder::parentFolder() const
{
	return m_parentId == m_id ? nullptr : m_store.lock()->OpenFolder(m_parentId);
}

std::shared_ptr<Folder> Folder::lookupSubFolder(const response::IdType& id)
{
	LoadSubFolders({});

	auto itr = m_subFolderIds->find(id);

	if (itr == m_subFolderIds->cend())
	{
		return nullptr;
	}

	return m_subFolders->at(itr->second);
}

const std::vector<std::shared_ptr<Item>>& Folder::items()
{
	LoadItems({});
	return *m_items;
}

std::shared_ptr<Item> Folder::lookupItem(const response::IdType& id)
{
	LoadItems({});

	auto itr = m_itemIds->find(id);

	if (itr == m_itemIds->cend())
	{
		return nullptr;
	}

	return m_items->at(itr->second);
}

const SPropValue& Folder::GetColumnProp(DefaultColumn column) const
{
	const auto index = static_cast<size_t>(column);

	CFRt(m_columnCount > static_cast<size_t>(index));

	return m_columns.get()[index];
}

response::IdType Folder::GetIdColumn(DefaultColumn column) const
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

std::string Folder::GetStringColumn(DefaultColumn column) const
{
	const auto& stringProp = GetColumnProp(column);

	if (PROP_TYPE(stringProp.ulPropTag) != PT_UNICODE)
	{
		return {};
	}

	return convert::utf8::to_utf8(stringProp.Value.lpszW);
}

int Folder::GetIntColumn(DefaultColumn column) const
{
	const auto& intProp = GetColumnProp(column);

	if (PROP_TYPE(intProp.ulPropTag) != PT_LONG)
	{
		return 0;
	}

	return static_cast<int>(intProp.Value.l);
}

void Folder::OpenFolder()
{
	if (m_folder)
	{
		return;
	}

	ULONG objType = 0;

	CORt(m_store.lock()->store()->OpenEntry(static_cast<ULONG>(m_id.size()),
		reinterpret_cast<LPENTRYID>(const_cast<response::IdType&>(m_id).data()),
		&IID_IMAPIFolder,
		MAPI_BEST_ACCESS | MAPI_DEFERRED_ERRORS,
		&objType,
		reinterpret_cast<LPUNKNOWN*>(&m_folder)));
	CFRt(m_folder != nullptr);
	CFRt(objType == MAPI_FOLDER);
}

void Folder::LoadSubFolders(service::Directives&& fieldDirectives)
{
	if (m_subFolderDirectives != fieldDirectives)
	{
		// Reset the subFolders and reload if the directives change
		m_subFolders.reset();
		m_subFolderDirectives = std::move(fieldDirectives);
	}

	if (m_subFolders)
	{
		return;
	}

	m_subFolderIds = std::make_unique<std::map<response::IdType, size_t>>();
	m_subFolders = std::make_unique<std::vector<std::shared_ptr<Folder>>>();

	constexpr auto c_folderProps = GetFolderColumns();
	mapi_ptr<SPropTagArray> folderProps;

	CORt(::MAPIAllocateBuffer(CbNewSPropTagArray(c_folderProps.size()),
		reinterpret_cast<void**>(&out_ptr { folderProps })));
	CFRt(folderProps != nullptr);
	folderProps->cValues = static_cast<ULONG>(c_folderProps.size());
	std::copy(c_folderProps.begin(), c_folderProps.end(), folderProps->aulPropTag);

	constexpr auto c_folderSorts = GetFolderSorts();
	mapi_ptr<SSortOrderSet> folderSorts;

	CORt(::MAPIAllocateBuffer(CbNewSSortOrderSet(c_folderSorts.size()),
		reinterpret_cast<void**>(&out_ptr { folderSorts })));
	CFRt(folderSorts != nullptr);
	folderSorts->cSorts = static_cast<ULONG>(c_folderSorts.size());
	folderSorts->cCategories = 0;
	folderSorts->cExpanded = 0;
	std::copy(c_folderSorts.begin(), c_folderSorts.end(), folderSorts->aSort);

	auto store = m_store.lock();
	const TableDirectives directives { store, m_subFolderDirectives };
	CComPtr<IMAPITable> sptable;

	CORt(folder()->GetHierarchyTable(MAPI_DEFERRED_ERRORS | MAPI_UNICODE, &sptable));

	const rowset_ptr sprows =
		directives.read(sptable, std::move(folderProps), std::move(folderSorts));

	m_subFolders->reserve(static_cast<size_t>(sprows->cRows));
	for (ULONG i = 0; i != sprows->cRows; i++)
	{
		auto& row = sprows->aRow[i];
		const size_t columnCount = static_cast<size_t>(row.cValues);
		mapi_ptr<SPropValue> columns { row.lpProps };

		row.lpProps = nullptr;

		auto folder = std::make_shared<Folder>(store, nullptr, columnCount, std::move(columns));

		store->CacheFolder(folder);
		m_subFolderIds->insert(std::make_pair(folder->id(), m_subFolders->size()));
		m_subFolders->push_back(std::move(folder));
	}

	if (!m_subFolderSink)
	{
		auto spThis = shared_from_this();
		CComPtr<AdviseSinkProxy<IMAPITable>> sinkProxy;
		ULONG_PTR connectionId = 0;

		sinkProxy.Attach(new AdviseSinkProxy<IMAPITable>(
			[wpFolder = std::weak_ptr { spThis }](size_t, LPNOTIFICATION) {
				auto spFolder = wpFolder.lock();

				if (spFolder)
				{
					spFolder->m_subFolders.reset();
				}
			}));

		CORt(sptable->Advise(fnevTableModified, sinkProxy, &connectionId));
		sinkProxy->OnAdvise(sptable, connectionId);

		m_subFolderSink = sinkProxy;
	}
}

void Folder::LoadItems(service::Directives&& fieldDirectives)
{
	if (m_itemDirectives != fieldDirectives)
	{
		// Reset the items and reload if the directives change
		m_items.reset();
		m_itemDirectives = std::move(fieldDirectives);
	}

	if (m_items)
	{
		return;
	}

	m_itemIds = std::make_unique<std::map<response::IdType, size_t>>();
	m_items = std::make_unique<std::vector<std::shared_ptr<Item>>>();

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

	auto store = m_store.lock();
	const TableDirectives directives { store, m_itemDirectives };
	CComPtr<IMAPITable> sptable;

	CORt(folder()->GetContentsTable(MAPI_DEFERRED_ERRORS | MAPI_UNICODE, &sptable));

	const rowset_ptr sprows = directives.read(sptable, std::move(itemProps), std::move(itemSorts));

	m_items->reserve(static_cast<size_t>(sprows->cRows));
	for (ULONG i = 0; i != sprows->cRows; i++)
	{
		auto& row = sprows->aRow[i];
		const size_t columnCount = static_cast<size_t>(row.cValues);
		mapi_ptr<SPropValue> columns { row.lpProps };

		row.lpProps = nullptr;

		auto item = std::make_shared<Item>(store, nullptr, columnCount, std::move(columns));

		store->CacheItem(item);
		m_itemIds->insert({ item->id(), m_items->size() });
		m_items->push_back(std::move(item));
	}

	if (!m_itemSink)
	{
		auto spThis = shared_from_this();
		CComPtr<AdviseSinkProxy<IMAPITable>> sinkProxy;
		ULONG_PTR connectionId = 0;

		sinkProxy.Attach(new AdviseSinkProxy<IMAPITable>(
			[wpFolder = std::weak_ptr { spThis }](size_t, LPNOTIFICATION) {
				auto spFolder = wpFolder.lock();

				if (spFolder)
				{
					spFolder->m_items.reset();
				}
			}));

		CORt(sptable->Advise(fnevTableModified, sinkProxy, &connectionId));
		sinkProxy->OnAdvise(sptable, connectionId);

		m_itemSink = sinkProxy;
	}
}

const response::IdType& Folder::getId() const
{
	return m_id;
}

std::shared_ptr<object::Folder> Folder::getParentFolder() const
{
	return std::make_shared<object::Folder>(parentFolder());
}

std::shared_ptr<object::Store> Folder::getStore() const
{
	return std::make_shared<object::Store>(m_store.lock());
}

const std::string& Folder::getName() const
{
	return m_name;
}

int Folder::getCount() const
{
	return m_count;
}

int Folder::getUnread() const
{
	return m_unread;
}

std::optional<SpecialFolder> Folder::getSpecialFolder() const
{
	return m_specialFolder;
}

std::vector<std::shared_ptr<object::Property>> Folder::getColumns() const
{
	auto store = m_store.lock();
	const auto offset = static_cast<size_t>(DefaultColumn::Count);

	CFRt(m_columnCount >= offset);
	return { store->GetColumns(m_columnCount - offset, m_columns.get() + offset) };
}

std::vector<std::shared_ptr<object::Folder>> Folder::getSubFolders(
	service::FieldParams&& params, std::optional<std::vector<response::IdType>>&& idsArg)
{
	std::vector<std::shared_ptr<object::Folder>> result {};

	LoadSubFolders(std::move(params.fieldDirectives));

	if (idsArg)
	{
		// Lookup the folders with the specified IDs
		result.resize(idsArg->size());
		std::transform(idsArg->cbegin(),
			idsArg->cend(),
			result.begin(),
			[this](const response::IdType& id) noexcept {
				return std::make_shared<object::Folder>(lookupSubFolder(id));
			});
	}
	else
	{
		// Just clone the entire vector of folders.
		result.resize(m_subFolders->size());
		std::transform(m_subFolders->cbegin(),
			m_subFolders->cend(),
			result.begin(),
			[this](const std::shared_ptr<Folder>& folder) noexcept {
				return std::make_shared<object::Folder>(folder);
			});
	}

	return result;
}

std::vector<std::shared_ptr<object::Conversation>> Folder::getConversations(
	service::FieldParams&& params, std::optional<std::vector<response::IdType>>&& idsArg)
{
	// NYI
	return {};
}

std::vector<std::shared_ptr<object::Item>> Folder::getItems(
	service::FieldParams&& params, std::optional<std::vector<response::IdType>>&& idsArg)
{
	std::vector<std::shared_ptr<object::Item>> result {};

	LoadItems(std::move(params.fieldDirectives));

	if (idsArg)
	{
		// Lookup the items with the specified IDs
		result.resize(idsArg->size());
		std::transform(idsArg->cbegin(),
			idsArg->cend(),
			result.begin(),
			[this](const response::IdType& id) noexcept {
				return std::make_shared<object::Item>(lookupItem(id));
			});
	}
	else
	{
		// Just clone the entire vector of items.
		result.resize(m_items->size());
		std::transform(m_items->cbegin(),
			m_items->cend(),
			result.begin(),
			[this](const std::shared_ptr<Item>& item) noexcept {
				return std::make_shared<object::Item>(item);
			});
	}

	return result;
}

} // namespace graphql::mapi