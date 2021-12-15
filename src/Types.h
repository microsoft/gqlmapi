// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#define NOMINMAX

#include "MAPISchema.h"

#include <windows.h>

// clang - format off
#pragma warning(disable : 26426) // Warning C26426 Global initializer calls a non-constexpr (i.22)
#pragma warning(disable : 26446) // Warning C26446 Prefer to use gsl::at() instead of unchecked
								 // subscript operator (bounds.4).
#pragma warning(                                                                                   \
	disable : 26481) // Warning C26481 Don't use pointer arithmetic. Use span instead (bounds.1).
#pragma warning(                                                                                   \
	disable : 26485) // Warning C26485 Expression '': No array to pointer decay (bounds.3).
#pragma warning(                                                                                   \
	disable : 26487) // Warning C26487 Don't return a pointer '' that may be invalid (lifetime.4).
// clang-format on

#include <atlbase.h>
#include <mapi.h>
#include <mapiutil.h>
#include <mapix.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <variant>

#include "CheckResult.h"
#include "Unicode.h"

namespace graphql::mapi {

// Wrapper for smart pointers to enable out-params by reference.
template <class SmartPointer>
class out_ptr
{
public:
	using UniquePointer =
		std::unique_ptr<typename SmartPointer::element_type, typename SmartPointer::deleter_type>;
	using RawPointer = typename SmartPointer::pointer;

	out_ptr(SmartPointer& ref)
		: m_ref { ref }
		, m_ptr { ref.release() }
	{
		static_assert(std::is_base_of_v<UniquePointer, SmartPointer>,
			"only valid for sub-classes of std::unique_ptr");
	}

	~out_ptr()
	{
		m_ref.reset(m_ptr);
	}

	RawPointer* operator&()
	{
		return &m_ptr;
	}

private:
	SmartPointer& m_ref;
	RawPointer m_ptr;
};

// MAPI smart-pointers implemented with std::unique_ptr.
class rowset_ptr : public std::unique_ptr<SRowSet, decltype(&::FreeProws)>
{
public:
	using UniquePointer = std::unique_ptr<SRowSet, decltype(&::FreeProws)>;

	rowset_ptr(LPSRowSet prows = nullptr) noexcept
		: UniquePointer { prows, ::FreeProws }
	{
	}
};

template <typename Type>
class mapi_ptr : public std::unique_ptr<Type, decltype(&::MAPIFreeBuffer)>
{
public:
	using UniquePointer = std::unique_ptr<Type, decltype(&::MAPIFreeBuffer)>;

	mapi_ptr(Type* ptr = nullptr) noexcept
		: UniquePointer { ptr, ::MAPIFreeBuffer }
	{
	}
};

// Shared MAPI session which lives as long as any of the service Operations
class Session : public std::enable_shared_from_this<Session>
{
public:
	explicit Session(bool useDefaultProfile);
	~Session();

	const CComPtr<IMAPISession>& session() const;

private:
	CComPtr<IMAPISession> m_session;
};

// Notification sink which will forward to a callback.
template <class _Interface>
class AdviseSinkProxy : public IMAPIAdviseSink
{
public:
	using Callback = std::function<void(size_t, LPNOTIFICATION)>;

	explicit AdviseSinkProxy(Callback&& callback)
		: m_callback { std::move(callback) }
	{
	}

	~AdviseSinkProxy()
	{
		Unadvise();
	}

	void OnAdvise(_Interface* source, ULONG_PTR connectionId)
	{
		m_source = source;
		m_connectionId = connectionId;
	}

	void Unadvise()
	{
		if (m_source)
		{
			m_source->Unadvise(m_connectionId);
			m_source.Release();
		}
	}

	// IUnknown
	STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj)
	{
		if (nullptr == ppvObj)
		{
			return E_POINTER;
		}

		if (riid == IID_IUnknown || riid == IID_IMAPIAdviseSink)
		{
			*ppvObj = reinterpret_cast<void*>(this);
			AddRef();
			return S_OK;
		}

		return E_NOINTERFACE;
	}

	STDMETHODIMP_(ULONG) AddRef()
	{
		return ++m_refcount;
	}

	STDMETHODIMP_(ULONG) Release()
	{
		const ULONG refcount = --m_refcount;

		if (refcount == 0)
		{
			delete this;
		}

		return refcount;
	}

	// IMAPIAdviseSink
	STDMETHODIMP_(ULONG) OnNotify(ULONG cNotif, LPNOTIFICATION lpNotifications)
	{
		m_callback(static_cast<size_t>(cNotif), lpNotifications);
		return S_OK;
	}

private:
	Callback m_callback;
	CComPtr<_Interface> m_source;
	ULONG_PTR m_connectionId { 0 };
	std::atomic<ULONG> m_refcount { 1 };
};

// Additional property tags which MAPIStubLibrary doesn't know about.
constexpr ULONG PR_CONVERSATION_ID = PROP_TAG(PT_BINARY,
	0x3013); // https://docs.microsoft.com/en-us/openspecs/exchange_server_protocols/ms-oxprops/7fdd0560-5e41-4518-bfbb-0c5a6eb6be6c

// Forward declarations
class Store;
class Folder;
class Item;
class Property;

class Query : public std::enable_shared_from_this<Query>
{
public:
	explicit Query(const std::shared_ptr<Session>& session);
	~Query();

	// Accessors used by other MAPIGraphQL classes
	const std::vector<std::shared_ptr<Store>>& stores();
	std::shared_ptr<Store> lookup(const response::IdType& id);

	// Clear cached folders and items in all stores.
	void ClearCaches();

	// Clear the folder and item caches at the end of the selection set.
	void endSelectionSet(const service::SelectionSetParams& params);

	// Resolvers/Accessors which implement the GraphQL type
	std::vector<std::shared_ptr<object::Store>> getStores(
		service::FieldParams&& params, std::optional<std::vector<response::IdType>>&& idsArg);

private:
	std::shared_ptr<Session> m_session;

	// These lazy load and cache results between calls to const methods.
	void LoadStores(service::Directives&& fieldDirectives);

	std::unique_ptr<std::map<response::IdType, size_t>> m_ids;
	std::unique_ptr<std::vector<std::shared_ptr<Store>>> m_stores;
	CComPtr<AdviseSinkProxy<IMAPITable>> m_storeSink;
	service::Directives m_storeDirectives;
};

class Mutation
{
public:
	explicit Mutation(const std::shared_ptr<Query>& query);
	~Mutation();

	// Accessors used by other MAPIGraphQL classes
	bool CopyItems(MultipleItemsInput&& inputArg, ObjectId&& destinationArg, bool moveItems);

	// Clear the folder and item caches at the end of the selection set.
	void endSelectionSet(const service::SelectionSetParams& params);

	// Resolvers/Accessors which implement the GraphQL type
	std::shared_ptr<object::Item> applyCreateItem(CreateItemInput&& inputArg);
	std::shared_ptr<object::Folder> applyCreateSubFolder(CreateSubFolderInput&& inputArg);
	std::shared_ptr<object::Item> applyModifyItem(ModifyItemInput&& inputArg);
	std::shared_ptr<object::Folder> applyModifyFolder(ModifyFolderInput&& inputArg);
	bool applyRemoveFolder(ObjectId&& inputArg, bool hardDeleteArg);
	bool applyMarkAsRead(MultipleItemsInput&& inputArg, bool readArg);
	bool applyCopyItems(MultipleItemsInput&& inputArg, ObjectId&& destinationArg);
	bool applyMoveItems(MultipleItemsInput&& inputArg, ObjectId&& destinationArg);
	bool applyDeleteItems(MultipleItemsInput&& inputArg, bool hardDeleteArg);

private:
	std::shared_ptr<Query> m_query;
};

class Subscription : public std::enable_shared_from_this<Subscription>
{
public:
	explicit Subscription(const std::shared_ptr<Query>& query);
	~Subscription();

	void setService(const std::shared_ptr<Operations>& service) noexcept;

	// Resolvers/Accessors which implement the GraphQL type
	std::vector<std::shared_ptr<object::ItemChange>> getItems(
		service::FieldParams&& params, ObjectId&& folderIdArg);
	std::vector<std::shared_ptr<object::FolderChange>> getSubFolders(
		service::FieldParams&& params, ObjectId&& parentFolderIdArg);
	std::vector<std::shared_ptr<object::FolderChange>> getRootFolders(
		service::FieldParams&& params, response::IdType&& storeIdArg);

private:
	// These are all initialized at construction.
	std::shared_ptr<Query> m_query;

	// Initialized after construction with setService.
	std::weak_ptr<Operations> m_service;

	struct RegistrationKey
	{
		// These form the key in each set of registrations.
		ObjectId objectId;
		service::Directives directives;

		// Enable < comparisons so it can be used a std::multiset.
		bool operator<(const RegistrationKey& rhs) const noexcept;
	};

	// If multiple subscriptions are registered with the same arguments and directives, they
	// should re-use the existing sink.
	template <class Row>
	struct TableSink : std::enable_shared_from_this<TableSink<Row>>
	{
		// This should be filled in with a callback which translates the table notifications
		// into appropriate subscription types and deliver those to the listeners on the service.
		CComPtr<AdviseSinkProxy<IMAPITable>> sinkProxy;

		// We need to hold on to the store to handle named properties.
		std::shared_ptr<Store> store;

		// We need to hold on to the table to perform reloads.
		CComPtr<IMAPITable> table;

		// Cache the window of rows to use for translating the table notifications.
		std::vector<std::shared_ptr<Row>> rows;
	};

	// Track the registration of listeners for a given table and set of table directives.
	template <class Row>
	struct Registration
	{
		const RegistrationKey key;

		// Enable < comparisons so it can be used a std::multiset.
		bool operator<(const Registration& rhs) const noexcept
		{
			return key < rhs.key;
		}

		std::shared_ptr<TableSink<Row>> sink;
	};

	template <class T, class ArgumentType, class PayloadType>
	void RegisterAdviseSinkProxy(service::await_async launch, std::string&& fieldName,
		std::string_view argumentName, const ArgumentType& argumentValue,
		Registration<T>& registration) const;

	template <class T>
	std::vector<std::shared_ptr<T>> LoadRows(const RegistrationKey& key,
		std::shared_ptr<Store>& store, CComPtr<IMAPITable>& spTable) const;

	// Specialized for Item and Folder
	template <>
	std::vector<std::shared_ptr<Item>> LoadRows<Item>(const RegistrationKey& key,
		std::shared_ptr<Store>& store, CComPtr<IMAPITable>& spTable) const;
	template <>
	std::vector<std::shared_ptr<Folder>> LoadRows<Folder>(const RegistrationKey& key,
		std::shared_ptr<Store>& store, CComPtr<IMAPITable>& spTable) const;

	// Using std::multiset because there could be multiple subscriptions on the same
	// ObjectId and table directives. In that case, they should all take a reference on
	// the same Registration::sink member and share a single IMAPIAdviseSink registration.
	mutable std::multiset<Registration<Item>> m_itemSinks;
	mutable std::multiset<Registration<Folder>> m_subFolderSinks;
	mutable std::multiset<Registration<Folder>> m_rootFolderSinks;
};

class TableDirectives
{
public:
	explicit TableDirectives(
		const std::shared_ptr<Store>& store, const service::Directives& fieldDirectives) noexcept;

	rowset_ptr read(IMAPITable* pTable, mapi_ptr<SPropTagArray>&& defaultColumns,
		mapi_ptr<SSortOrderSet>&& defaultOrder = {}) const;

private:
	mapi_ptr<SPropTagArray> columns(mapi_ptr<SPropTagArray>&& defaultColumns) const;
	mapi_ptr<SSortOrderSet> orderBy(mapi_ptr<SSortOrderSet>&& defaultOrder) const;
	mapi_ptr<SRestriction> seek() const;
	BOOKMARK seekBookmark() const;
	LONG offset() const;
	LONG take() const;

	const std::shared_ptr<Store> m_store;
	const std::optional<std::vector<Column>> m_columns;
	const std::optional<std::vector<Order>> m_orderBy;
	const std::optional<std::optional<response::IdType>> m_seek;
	const std::optional<int> m_offset;
	const std::optional<int> m_take;
};

struct CompareMAPINAMEID
{
	bool operator()(
		const mapi_ptr<MAPINAMEID>& lhs, const mapi_ptr<MAPINAMEID>& rhs) const noexcept;
};

using NameIdToPropId = std::map<mapi_ptr<MAPINAMEID>, ULONG, CompareMAPINAMEID>;

class Store : public std::enable_shared_from_this<Store>
{
public:
	explicit Store(
		const CComPtr<IMAPISession>& session, size_t columnCount, mapi_ptr<SPropValue>&& columns);
	~Store();

	// Accessors used by other MAPIGraphQL classes
	enum class DefaultColumn : size_t
	{
		Id,
		Name,
		Count
	};

	static constexpr std::array<ULONG, static_cast<size_t>(DefaultColumn::Count)> GetStoreColumns()
	{
		constexpr std::array storeProps {
			PR_ENTRYID,
			PR_DISPLAY_NAME_W,
		};

		return storeProps;
	};

	static constexpr std::array<SSortOrder, 1> GetStoreSorts()
	{
		return { SSortOrder { PR_DISPLAY_NAME_W, TABLE_SORT_ASCEND } };
	}

	const CComPtr<IMsgStore>& store();
	const response::IdType& id() const;
	const response::IdType& rootId() const;
	const std::vector<std::shared_ptr<Folder>>& rootFolders();
	std::shared_ptr<Folder> lookupRootFolder(const response::IdType& id);
	const std::map<SpecialFolder, response::IdType>& specialFolders();
	std::shared_ptr<Folder> lookupSpecialFolder(SpecialFolder id);
	std::vector<std::pair<ULONG, LPMAPINAMEID>> lookupPropIdInputs(
		std::vector<PropIdInput>&& namedProps);
	std::vector<std::pair<ULONG, LPMAPINAMEID>> lookupPropIds(const std::vector<ULONG>& propIds);

	// Utility methods which help with converting between input types and MAPI types
	std::vector<std::shared_ptr<object::Property>> GetColumns(
		size_t columnCount, const LPSPropValue columns);
	std::vector<std::shared_ptr<object::Property>> GetProperties(
		IMAPIProp* pObject, std::optional<std::vector<Column>>&& idsArg);
	void ConvertPropertyInputs(void* pAllocMore, LPSPropValue propBegin, LPSPropValue propEnd,
		std::vector<PropertyInput>&& input);

	// Open and cache folders and items
	std::shared_ptr<Folder> OpenFolder(const response::IdType& folderId);
	std::shared_ptr<Item> OpenItem(const response::IdType& itemId);
	void CacheFolder(const std::shared_ptr<Folder>& folder);
	void CacheItem(const std::shared_ptr<Item>& item);
	void ClearCaches();

	// Resolvers/Accessors which implement the GraphQL type
	const response::IdType& getId() const;
	const std::string& getName() const;
	std::vector<std::shared_ptr<object::Property>> getColumns();
	std::vector<std::shared_ptr<object::Folder>> getRootFolders(
		service::FieldParams&& params, std::optional<std::vector<response::IdType>>&& idsArg);
	std::vector<std::shared_ptr<object::Folder>> getSpecialFolders(
		std::vector<SpecialFolder>&& idsArg);
	std::vector<std::shared_ptr<object::Property>> getFolderProperties(
		response::IdType&& folderIdArg, std::optional<std::vector<Column>>&& idsArg);
	std::vector<std::shared_ptr<object::Property>> getItemProperties(
		response::IdType&& itemIdArg, std::optional<std::vector<Column>>&& idsArg);

private:
	// Used during construction
	const SPropValue& GetColumnProp(DefaultColumn column) const;
	response::IdType GetIdColumn(DefaultColumn column) const;
	std::string GetStringColumn(DefaultColumn column) const;

	// These are all initialized at construction.
	const CComPtr<IMAPISession> m_session;
	const size_t m_columnCount;
	const mapi_ptr<SPropValue> m_columns;
	const response::IdType m_id;
	const std::string m_name;

	// These lazy load and cache results between calls to const methods.
	void OpenStore();
	void LoadSpecialFolders();
	void LoadRootFolders(service::Directives&& fieldDirectives);
	mapi_ptr<SPropTagArray> GetFolderProperties() const;
	mapi_ptr<SPropTagArray> GetItemProperties() const;

	// Utility methods to populate our cached special folder IDs from multiple properties on the
	// store and folders.
	static void FillInStoreProps(LPSPropValue storeIds, std::map<SpecialFolder, SBinary>& idMap);
	static void FillInFolderProps(LPSPropValue folderIds, std::map<SpecialFolder, SBinary>& idMap);

	CComPtr<IMsgStore> m_store;
	response::IdType m_rootId;
	CComPtr<IMAPIFolder> m_ipmSubtree;
	mapi_ptr<SPropValue> m_storeProps;
	mapi_ptr<SPropValue> m_ipmSubtreeProps;
	mapi_ptr<SPropValue> m_inboxProps;
	ULONG m_cbInboxId = 0;
	mapi_ptr<ENTRYID> m_eidInboxId;
	std::unique_ptr<std::vector<std::shared_ptr<Folder>>> m_rootFolders;
	std::unique_ptr<std::map<response::IdType, size_t>> m_rootFolderIds;
	CComPtr<AdviseSinkProxy<IMAPITable>> m_rootFolderSink;
	service::Directives m_rootFolderDirectives;
	std::unique_ptr<std::map<SpecialFolder, response::IdType>> m_specialFolders;
	NameIdToPropId m_nameIdToPropIds;
	std::map<response::IdType, std::shared_ptr<Folder>> m_folderCache;
	std::map<response::IdType, std::shared_ptr<Item>> m_itemCache;
};

class Folder : public std::enable_shared_from_this<Folder>
{
public:
	explicit Folder(const std::shared_ptr<Store>& store, IMAPIFolder* pFolder, size_t columnCount,
		mapi_ptr<SPropValue>&& columns);
	~Folder();

	// Accessors used by other MAPIGraphQL classes
	enum class DefaultColumn : size_t
	{
		InstanceKey,
		Id,
		ParentId,
		Name,
		Total,
		Unread,
		Count
	};

	static constexpr std::array<ULONG, static_cast<size_t>(DefaultColumn::Count)> GetFolderColumns()
	{
		constexpr std::array folderProps {
			PR_INSTANCE_KEY,
			PR_ENTRYID,
			PR_PARENT_ENTRYID,
			PR_DISPLAY_NAME_W,
			PR_CONTENT_COUNT,
			PR_CONTENT_UNREAD,
		};

		return folderProps;
	};

	static constexpr std::array<SSortOrder, 1> GetFolderSorts()
	{
		return { SSortOrder { PR_DISPLAY_NAME_W, TABLE_SORT_ASCEND } };
	}

	const response::IdType& instanceKey() const;
	const response::IdType& id() const;
	const std::string& name() const;
	int count() const;
	int unread() const;
	const CComPtr<IMAPIFolder>& folder();
	const std::vector<std::shared_ptr<Folder>>& subFolders();
	std::shared_ptr<Folder> parentFolder() const;
	std::shared_ptr<Folder> lookupSubFolder(const response::IdType& id);
	const std::vector<std::shared_ptr<Item>>& items();
	std::shared_ptr<Item> lookupItem(const response::IdType& id);

	// Resolvers/Accessors which implement the GraphQL type
	const response::IdType& getId() const;
	std::shared_ptr<object::Folder> getParentFolder() const;
	std::shared_ptr<object::Store> getStore() const;
	const std::string& getName() const;
	int getCount() const;
	int getUnread() const;
	std::optional<SpecialFolder> getSpecialFolder() const;
	std::vector<std::shared_ptr<object::Property>> getColumns() const;
	std::vector<std::shared_ptr<object::Folder>> getSubFolders(
		service::FieldParams&& params, std::optional<std::vector<response::IdType>>&& idsArg);
	std::vector<std::shared_ptr<object::Conversation>> getConversations(
		service::FieldParams&& params, std::optional<std::vector<response::IdType>>&& idsArg);
	std::vector<std::shared_ptr<object::Item>> getItems(
		service::FieldParams&& params, std::optional<std::vector<response::IdType>>&& idsArg);

private:
	// Used during construction
	const SPropValue& GetColumnProp(DefaultColumn column) const;
	response::IdType GetIdColumn(DefaultColumn column) const;
	std::string GetStringColumn(DefaultColumn column) const;
	int GetIntColumn(DefaultColumn column) const;

	// These are all initialized at construction.
	const std::weak_ptr<Store> m_store;
	const size_t m_columnCount;
	const mapi_ptr<SPropValue> m_columns;
	const response::IdType m_instanceKey;
	const response::IdType m_id;
	const response::IdType m_parentId;
	const std::string m_name;
	const int m_count;
	const int m_unread;
	const std::optional<SpecialFolder> m_specialFolder;

	// These lazy load and cache results between calls to const methods.
	void OpenFolder();
	void LoadSubFolders(service::Directives&& fieldDirectives);
	void LoadItems(service::Directives&& fieldDirectives);

	CComPtr<IMAPIFolder> m_folder;
	std::unique_ptr<std::map<response::IdType, size_t>> m_subFolderIds;
	std::unique_ptr<std::vector<std::shared_ptr<Folder>>> m_subFolders;
	CComPtr<AdviseSinkProxy<IMAPITable>> m_subFolderSink;
	service::Directives m_subFolderDirectives;
	std::unique_ptr<std::map<response::IdType, size_t>> m_itemIds;
	std::unique_ptr<std::vector<std::shared_ptr<Item>>> m_items;
	CComPtr<AdviseSinkProxy<IMAPITable>> m_itemSink;
	service::Directives m_itemDirectives;
};

class Item : public std::enable_shared_from_this<Item>
{
public:
	explicit Item(const std::shared_ptr<Store>& store, IMessage* pMessage, size_t columnCount,
		mapi_ptr<SPropValue>&& columns);

	// Accessors used by other MAPIGraphQL classes
	enum class DefaultColumn : size_t
	{
		InstanceKey,
		Id,
		ParentId,
		Subject,
		Sender,
		To,
		Cc,
		MessageFlags,
		Received,
		Modified,
		Preview,
		Count
	};

	static constexpr std::array<ULONG, static_cast<size_t>(DefaultColumn::Count)> GetItemColumns()
	{
		return {
			PR_INSTANCE_KEY,
			PR_ENTRYID,
			PR_PARENT_ENTRYID,
			PR_SUBJECT_W,
			PR_SENDER_NAME_W,
			PR_DISPLAY_TO_W,
			PR_DISPLAY_CC_W,
			PR_MESSAGE_FLAGS,
			PR_MESSAGE_DELIVERY_TIME,
			PR_LAST_MODIFICATION_TIME,
			PR_BODY_W,
		};
	};

	static constexpr std::array<SSortOrder, 1> GetItemSorts()
	{
		return { SSortOrder { PR_MESSAGE_DELIVERY_TIME, TABLE_SORT_DESCEND } };
	}

	const response::IdType& instanceKey() const;
	const response::IdType& id() const;
	const std::string& subject() const;
	const std::optional<std::string>& sender() const;
	const std::optional<std::string>& to() const;
	const std::optional<std::string>& cc() const;
	bool read() const;
	const FILETIME& received() const;
	const FILETIME& modified() const;
	const std::optional<std::string>& preview() const;
	const CComPtr<IMessage>& message();

	// Resolvers/Accessors which implement the GraphQL type
	const response::IdType& getId() const;
	std::shared_ptr<object::Folder> getParentFolder() const;
	std::shared_ptr<object::Conversation> getConversation(service::FieldParams&& params) const;
	const std::string& getSubject() const;
	std::optional<std::string> getSender() const;
	std::optional<std::string> getTo() const;
	std::optional<std::string> getCc() const;
	std::optional<response::Value> getBody(service::FieldParams&& params) const;
	bool getRead() const;
	std::optional<response::Value> getReceived() const;
	std::optional<response::Value> getModified() const;
	std::optional<std::string> getPreview() const;
	std::vector<std::shared_ptr<object::Property>> getColumns() const;
	std::vector<std::shared_ptr<object::Attachment>> getAttachments(
		service::FieldParams&& params, std::optional<std::vector<response::IdType>>&& idsArg) const;

private:
	// Used during construction
	const SPropValue& GetColumnProp(DefaultColumn column) const;
	response::IdType GetIdColumn(DefaultColumn column) const;
	std::string GetStringColumn(DefaultColumn column) const;
	bool GetReadColumn(DefaultColumn column) const;
	FILETIME GetTimeColumn(DefaultColumn column) const;

	// These are all initialized at construction.
	const std::weak_ptr<Store> m_store;
	const size_t m_columnCount;
	const mapi_ptr<SPropValue> m_columns;
	const response::IdType m_instanceKey;
	const response::IdType m_id;
	const response::IdType m_parentId;
	const std::string m_subject;
	const std::optional<std::string> m_sender;
	const std::optional<std::string> m_to;
	const std::optional<std::string> m_cc;
	const bool m_read;
	const FILETIME m_received;
	const FILETIME m_modified;
	const std::optional<std::string> m_preview;

	// These lazy load and cache results between calls to const methods.
	void OpenItem();

	CComPtr<IMessage> m_message;
};

class Property
{
public:
	using id_variant = std::variant<ULONG, MAPINAMEID>;

	explicit Property(const id_variant& id, mapi_ptr<SPropValue>&& value);

	// Resolvers/Accessors which implement the GraphQL type
	std::shared_ptr<object::PropId> getId() const;
	std::shared_ptr<object::PropValue> getValue() const;

private:
	const std::shared_ptr<object::PropId> m_id;
	const mapi_ptr<SPropValue> m_value;
};

class IntId
{
public:
	explicit IntId(ULONG id);

	// Resolvers/Accessors which implement the GraphQL type
	int getId() const;

private:
	const ULONG m_id;
};

class NamedId
{
public:
	explicit NamedId(const MAPINAMEID& named);

	// Resolvers/Accessors which implement the GraphQL type
	response::Value getPropset() const;
	std::shared_ptr<object::NamedPropId> getId() const;

private:
	const GUID m_propset;
	const std::shared_ptr<object::NamedPropId> m_named;
};

class StringId
{
public:
	explicit StringId(PCWSTR name);

	// Resolvers/Accessors which implement the GraphQL type
	const std::string& getName() const;

private:
	const std::string m_name;
};

class IntValue
{
public:
	explicit IntValue(int value);

	// Resolvers/Accessors which implement the GraphQL type
	int getValue() const;

private:
	const int m_value;
};

class BoolValue
{
public:
	explicit BoolValue(bool value);

	// Resolvers/Accessors which implement the GraphQL type
	bool getValue() const;

private:
	const bool m_value;
};

class StringValue
{
public:
	explicit StringValue(PCWSTR value);
	explicit StringValue(std::string&& value);

	// Resolvers/Accessors which implement the GraphQL type
	const std::string& getValue() const;

private:
	const std::string m_value;
};

class GuidValue
{
public:
	explicit GuidValue(const GUID& value);

	// Resolvers/Accessors which implement the GraphQL type
	response::Value getValue() const;

private:
	const GUID m_value;
};

class DateTimeValue
{
public:
	explicit DateTimeValue(const FILETIME& value);

	// Resolvers/Accessors which implement the GraphQL type
	response::Value getValue() const;

private:
	const FILETIME m_value;
};

class BinaryValue
{
public:
	explicit BinaryValue(const SBinary& value);

	// Resolvers/Accessors which implement the GraphQL type
	const response::IdType& getValue() const;

private:
	const response::IdType m_value;
};

class ItemAdded
{
public:
	explicit ItemAdded(int index, const std::shared_ptr<Item>& added);

	// Resolvers/Accessors which implement the GraphQL type
	int getIndex() const;
	std::shared_ptr<object::Item> getAdded() const;

private:
	const int m_index;
	const std::shared_ptr<Item> m_added;
};

class ItemUpdated
{
public:
	explicit ItemUpdated(int index, const std::shared_ptr<Item>& updated);

	// Resolvers/Accessors which implement the GraphQL type
	int getIndex() const;
	std::shared_ptr<object::Item> getUpdated() const;

private:
	const int m_index;
	const std::shared_ptr<Item> m_updated;
};

class ItemRemoved
{
public:
	explicit ItemRemoved(int index, const response::IdType& removed);

	int getIndex() const;
	const response::IdType& getRemoved() const;

private:
	const int m_index;
	const response::IdType m_removed;
};

class ItemsReloaded
{
public:
	explicit ItemsReloaded(const std::vector<std::shared_ptr<Item>>& reloaded);

	// Resolvers/Accessors which implement the GraphQL type
	std::vector<std::shared_ptr<object::Item>> getReloaded() const;

private:
	const std::vector<std::shared_ptr<Item>> m_reloaded;
};

class ItemsSubscription
{
public:
	explicit ItemsSubscription(std::vector<std::shared_ptr<object::ItemChange>>&& items);

	// Resolvers/Accessors which implement the GraphQL type
	std::vector<std::shared_ptr<object::ItemChange>> getItems(ObjectId&& folderIdArg) const;
	std::vector<std::shared_ptr<object::FolderChange>> getSubFolders(
		ObjectId&& parentFolderIdArg) const;
	std::vector<std::shared_ptr<object::FolderChange>> getRootFolders(
		response::IdType&& storeIdArg) const;

private:
	// These are all initialized at construction.
	std::vector<std::shared_ptr<object::ItemChange>> m_items;
};

class FolderAdded
{
public:
	explicit FolderAdded(int index, const std::shared_ptr<Folder>& added);

	// Resolvers/Accessors which implement the GraphQL type
	int getIndex() const;
	std::shared_ptr<object::Folder> getAdded() const;

private:
	const int m_index;
	const std::shared_ptr<Folder> m_added;
};

class FolderUpdated
{
public:
	explicit FolderUpdated(int index, const std::shared_ptr<Folder>& updated);

	// Resolvers/Accessors which implement the GraphQL type
	int getIndex() const;
	std::shared_ptr<object::Folder> getUpdated() const;

private:
	const int m_index;
	const std::shared_ptr<Folder> m_updated;
};

class FolderRemoved
{
public:
	explicit FolderRemoved(int index, const response::IdType& removed);

	// Resolvers/Accessors which implement the GraphQL type
	int getIndex() const;
	const response::IdType& getRemoved() const;

private:
	const int m_index;
	const response::IdType m_removed;
};

class FoldersReloaded
{
public:
	explicit FoldersReloaded(const std::vector<std::shared_ptr<Folder>>& reloaded);

	// Resolvers/Accessors which implement the GraphQL type
	std::vector<std::shared_ptr<object::Folder>> getReloaded() const;

private:
	const std::vector<std::shared_ptr<Folder>> m_reloaded;
};

class SubFoldersSubscription
{
public:
	explicit SubFoldersSubscription(
		std::vector<std::shared_ptr<object::FolderChange>>&& subFolders);

	// Resolvers/Accessors which implement the GraphQL type
	std::vector<std::shared_ptr<object::ItemChange>> getItems(ObjectId&& folderIdArg) const;
	std::vector<std::shared_ptr<object::FolderChange>> getSubFolders(
		ObjectId&& parentFolderIdArg) const;
	std::vector<std::shared_ptr<object::FolderChange>> getRootFolders(
		response::IdType&& storeIdArg) const;

private:
	// These are all initialized at construction.
	std::vector<std::shared_ptr<object::FolderChange>> m_subFolders;
};

class RootFoldersSubscription
{
public:
	explicit RootFoldersSubscription(
		std::vector<std::shared_ptr<object::FolderChange>>&& rootFolders);

	// Resolvers/Accessors which implement the GraphQL type
	std::vector<std::shared_ptr<object::ItemChange>> getItems(ObjectId&& folderIdArg) const;
	std::vector<std::shared_ptr<object::FolderChange>> getSubFolders(
		ObjectId&& parentFolderIdArg) const;
	std::vector<std::shared_ptr<object::FolderChange>> getRootFolders(
		response::IdType&& storeIdArg) const;

private:
	// These are all initialized at construction.
	std::vector<std::shared_ptr<object::FolderChange>> m_rootFolders;
};

} // namespace graphql::mapi