// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#define NOMINMAX

#include "MAPIObjects.h"

#include <windows.h>

// clang - format off
#pragma warning(disable : 26426) // Warning C26426 Global initializer calls a non-constexpr (i.22)
#pragma warning(disable : 26446) // Warning C26446 Prefer to use gsl::at() instead of unchecked subscript operator (bounds.4).
#pragma warning(disable : 26481) // Warning C26481 Don't use pointer arithmetic. Use span instead (bounds.1).
#pragma warning(disable : 26485) // Warning C26485 Expression '': No array to pointer decay (bounds.3).
#pragma warning(disable : 26487) // Warning C26487 Don't return a pointer '' that may be invalid (lifetime.4).
// clang-format on

#include <mapix.h>
#include <mapiutil.h>
#include <mapi.h>
#include <atlbase.h>

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
	using UniquePointer = std::unique_ptr<typename SmartPointer::element_type, typename SmartPointer::deleter_type>;
	using RawPointer = typename SmartPointer::pointer;

	out_ptr(SmartPointer& ref)
		: m_ref{ ref }
		, m_ptr{ ref.release() }
	{
		static_assert(std::is_base_of_v<UniquePointer, SmartPointer>, "only valid for sub-classes of std::unique_ptr");
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
		: UniquePointer{ prows, ::FreeProws }
	{
	}
};

template<typename Type>
class mapi_ptr : public std::unique_ptr<Type, decltype(&::MAPIFreeBuffer)>
{
public:
	using UniquePointer = std::unique_ptr<Type, decltype(&::MAPIFreeBuffer)>;

	mapi_ptr(Type* ptr = nullptr) noexcept
		: UniquePointer{ ptr, ::MAPIFreeBuffer }
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
		: m_callback{ std::move(callback) }
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

		if (riid == IID_IUnknown
			|| riid == IID_IMAPIAdviseSink)
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
	ULONG_PTR m_connectionId{ 0 };
	std::atomic<ULONG> m_refcount{ 1 };
};

// Additional property tags which MAPIStubLibrary doesn't know about.
constexpr ULONG PR_CONVERSATION_ID = PROP_TAG(PT_BINARY, 0x3013);

// Forward declarations
class Store;
class Folder;
class Item;
class Property;

class Query : public object::Query
{
public:
	explicit Query(const std::shared_ptr<Session>& session);
	~Query() override;

	// Accessors used by other MAPIGraphQL classes
	const std::vector<std::shared_ptr<Store>>& stores() const;
	std::shared_ptr<Store> lookup(const response::IdType& id) const;

	// Clear cached folders and items in all stores.
	void ClearCaches() const;

private:
	// Clear the folder and item caches at the end of the selection set.
	void endSelectionSet(const service::SelectionSetParams& params) const override;

	// Resolvers/Accessors which implement the GraphQL type
	service::FieldResult<std::vector<std::shared_ptr<object::Store>>> getStores(service::FieldParams&& params, std::optional<std::vector<response::IdType>>&& idsArg) const override;

	std::shared_ptr<Session> m_session;

	// These lazy load and cache results between calls to const methods.
	void LoadStores(response::Value&& fieldDirectives) const;

	mutable std::unique_ptr<std::map<response::IdType, size_t>> m_ids;
	mutable std::unique_ptr<std::vector<std::shared_ptr<Store>>> m_stores;
	mutable CComPtr<AdviseSinkProxy<IMAPITable>> m_storeSink;
	mutable response::Value m_storeDirectives;
};

class Mutation : public object::Mutation
{
public:
	explicit Mutation(const std::shared_ptr<Query>& query);
	~Mutation() override;

	// Accessors used by other MAPIGraphQL classes
	bool CopyItems(MultipleItemsInput&& inputArg, ObjectId&& destinationArg, bool moveItems) const;

private:
	// Clear the folder and item caches at the end of the selection set.
	void endSelectionSet(const service::SelectionSetParams& params) const override;

	// Resolvers/Accessors which implement the GraphQL type
	service::FieldResult<std::shared_ptr<object::Item>> applyCreateItem(service::FieldParams&& params, CreateItemInput&& inputArg) const override;
	service::FieldResult<std::shared_ptr<object::Folder>> applyCreateSubFolder(service::FieldParams&& params, CreateSubFolderInput&& inputArg) const override;
	service::FieldResult<std::shared_ptr<object::Item>> applyModifyItem(service::FieldParams&& params, ModifyItemInput&& inputArg) const override;
	service::FieldResult<std::shared_ptr<object::Folder>> applyModifyFolder(service::FieldParams&& params, ModifyFolderInput&& inputArg) const override;
	service::FieldResult<response::BooleanType> applyRemoveFolder(service::FieldParams&& params, ObjectId&& inputArg, response::BooleanType&& hardDeleteArg) const override;
	service::FieldResult<response::BooleanType> applyMarkAsRead(service::FieldParams&& params, MultipleItemsInput&& inputArg, response::BooleanType&& readArg) const override;
	service::FieldResult<response::BooleanType> applyCopyItems(service::FieldParams&& params, MultipleItemsInput&& inputArg, ObjectId&& destinationArg) const override;
	service::FieldResult<response::BooleanType> applyMoveItems(service::FieldParams&& params, MultipleItemsInput&& inputArg, ObjectId&& destinationArg) const override;
	service::FieldResult<response::BooleanType> applyDeleteItems(service::FieldParams&& params, MultipleItemsInput&& inputArg, response::BooleanType&& hardDeleteArg) const override;

	std::shared_ptr<Query> m_query;
};

class Subscription : public object::Subscription
{
public:
	explicit Subscription(const std::shared_ptr<Query>& query);
	~Subscription() override;

	void setService(const std::shared_ptr<Operations>& service) noexcept;

private:
	// Resolvers/Accessors which implement the GraphQL type
	service::FieldResult<std::vector<std::shared_ptr<service::Object>>> getItems(service::FieldParams&& params, ObjectId&& folderIdArg) const override;
	service::FieldResult<std::vector<std::shared_ptr<service::Object>>> getSubFolders(service::FieldParams&& params, ObjectId&& parentFolderIdArg) const override;
	service::FieldResult<std::vector<std::shared_ptr<service::Object>>> getRootFolders(service::FieldParams&& params, response::IdType&& storeIdArg) const override;

	// These are all initialized at construction.
	std::shared_ptr<Query> m_query;

	// Initialized after construction with setService.
	std::weak_ptr<Operations> m_service;

	struct RegistrationKey
	{
		// These form the key in each set of registrations.
		ObjectId objectId;
		response::Value directives;

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
	void RegisterAdviseSinkProxy(std::launch launch, std::string&& fieldName,
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
	explicit TableDirectives(const std::shared_ptr<const Store>& store, const response::Value& fieldDirectives) noexcept;

	rowset_ptr read(IMAPITable* pTable, mapi_ptr<SPropTagArray>&& defaultColumns, mapi_ptr<SSortOrderSet>&& defaultOrder = {}) const;

private:
	mapi_ptr<SPropTagArray> columns(mapi_ptr<SPropTagArray>&& defaultColumns) const;
	mapi_ptr<SSortOrderSet> orderBy(mapi_ptr<SSortOrderSet>&& defaultOrder) const;
	mapi_ptr<SRestriction> seek() const;
	BOOKMARK seekBookmark() const;
	LONG offset() const;
	LONG take() const;

	const std::shared_ptr<const Store> m_store;
	const std::optional<std::vector<Column>> m_columns;
	const std::optional<std::vector<Order>> m_orderBy;
	const std::optional<std::optional<response::IdType>> m_seek;
	const std::optional<response::IntType> m_offset;
	const std::optional<response::IntType> m_take;
};

struct CompareMAPINAMEID
{
	bool operator()(const mapi_ptr<MAPINAMEID>& lhs, const mapi_ptr<MAPINAMEID>& rhs) const noexcept;
};

using NameIdToPropId = std::map<mapi_ptr<MAPINAMEID>, ULONG, CompareMAPINAMEID>;

class Store : public object::Store
{
public:
	explicit Store(const CComPtr<IMAPISession>& session, size_t columnCount, mapi_ptr<SPropValue>&& columns);
	~Store() override;

	// Accessors used by other MAPIGraphQL classes
	enum class DefaultColumn : size_t {
		Id,
		Name,
		Count
	};

	static constexpr std::array<ULONG, static_cast<size_t>(DefaultColumn::Count)> GetStoreColumns()
	{
		constexpr std::array storeProps{
			PR_ENTRYID,
			PR_DISPLAY_NAME_W,
		};

		return storeProps;
	};

	static constexpr std::array<SSortOrder, 1> GetStoreSorts()
	{
		return {
			SSortOrder { PR_DISPLAY_NAME_W, TABLE_SORT_ASCEND }
		};
	}

	const CComPtr<IMsgStore>& store() const;
	const response::IdType& id() const;
	const response::IdType& rootId() const;
	const std::vector<std::shared_ptr<Folder>>& rootFolders() const;
	std::shared_ptr<Folder> lookupRootFolder(const response::IdType& id) const;
	const std::map<SpecialFolder, response::IdType>& specialFolders() const;
	std::shared_ptr<Folder> lookupSpecialFolder(SpecialFolder id) const;
	std::vector<std::pair<ULONG, LPMAPINAMEID>> lookupPropIdInputs(std::vector<PropIdInput>&& namedProps) const;
	std::vector<std::pair<ULONG, LPMAPINAMEID>> lookupPropIds(const std::vector<ULONG>& propIds) const;

	// Utility methods which help with converting between input types and MAPI types
	std::vector<std::shared_ptr<object::Property>> GetColumns(size_t columnCount, const LPSPropValue columns) const;
	std::vector<std::shared_ptr<object::Property>> GetProperties(IMAPIProp* pObject, std::optional<std::vector<Column>>&& idsArg) const;
	void ConvertPropertyInputs(void* pAllocMore, LPSPropValue propBegin, LPSPropValue propEnd, std::vector<PropertyInput>&& input) const;

	// Open and cache folders and items
	std::shared_ptr<Folder> OpenFolder(const response::IdType& folderId) const;
	std::shared_ptr<Item> OpenItem(const response::IdType& itemId) const;
	void CacheFolder(const std::shared_ptr<Folder>& folder) const;
	void CacheItem(const std::shared_ptr<Item>& item) const;
	void ClearCaches() const;

private:
	// Resolvers/Accessors which implement the GraphQL type
	service::FieldResult<response::IdType> getId(service::FieldParams&& params) const override;
	service::FieldResult<response::StringType> getName(service::FieldParams&& params) const override;
	service::FieldResult<std::vector<std::shared_ptr<object::Property>>> getColumns(service::FieldParams&& params) const override;
	service::FieldResult<std::vector<std::shared_ptr<object::Folder>>> getRootFolders(service::FieldParams&& params, std::optional<std::vector<response::IdType>>&& idsArg) const override;
	service::FieldResult<std::vector<std::shared_ptr<object::Folder>>> getSpecialFolders(service::FieldParams&& params, std::vector<SpecialFolder>&& idsArg) const override;
	service::FieldResult<std::vector<std::shared_ptr<object::Property>>> getFolderProperties(service::FieldParams&& params, response::IdType&& folderIdArg, std::optional<std::vector<Column>>&& idsArg) const override;
	service::FieldResult<std::vector<std::shared_ptr<object::Property>>> getItemProperties(service::FieldParams&& params, response::IdType&& itemIdArg, std::optional<std::vector<Column>>&& idsArg) const override;

	// Used during construction
	const SPropValue& GetColumnProp(DefaultColumn column) const;
	response::IdType GetIdColumn(DefaultColumn column) const;
	response::StringType GetStringColumn(DefaultColumn column) const;

	// These are all initialized at construction.
	const CComPtr<IMAPISession> m_session;
	const size_t m_columnCount;
	const mapi_ptr<SPropValue> m_columns;
	const response::IdType m_id;
	const response::StringType m_name;

	// These lazy load and cache results between calls to const methods.
	void OpenStore() const;
	void LoadSpecialFolders() const;
	void LoadRootFolders(response::Value&& fieldDirectives) const;
	mapi_ptr<SPropTagArray> GetFolderProperties() const;
	mapi_ptr<SPropTagArray> GetItemProperties() const;

	// Utility methods to populate our cached special folder IDs from multiple properties on the store and folders.
	static void FillInStoreProps(LPSPropValue storeIds, std::map<SpecialFolder, SBinary>& idMap);
	static void FillInFolderProps(LPSPropValue folderIds, std::map<SpecialFolder, SBinary>& idMap);

	mutable CComPtr<IMsgStore> m_store;
	mutable response::IdType m_rootId;
	mutable CComPtr<IMAPIFolder> m_ipmSubtree;
	mutable mapi_ptr<SPropValue> m_storeProps;
	mutable mapi_ptr<SPropValue> m_ipmSubtreeProps;
	mutable mapi_ptr<SPropValue> m_inboxProps;
	mutable ULONG m_cbInboxId = 0;
	mutable mapi_ptr<ENTRYID> m_eidInboxId;
	mutable std::unique_ptr<std::vector<std::shared_ptr<Folder>>> m_rootFolders;
	mutable std::unique_ptr<std::map<response::IdType, size_t>> m_rootFolderIds;
	mutable CComPtr<AdviseSinkProxy<IMAPITable>> m_rootFolderSink;
	mutable response::Value m_rootFolderDirectives;
	mutable std::unique_ptr<std::map<SpecialFolder, response::IdType>> m_specialFolders;
	mutable NameIdToPropId m_nameIdToPropIds;
	mutable std::map<response::IdType, std::shared_ptr<Folder>> m_folderCache;
	mutable std::map<response::IdType, std::shared_ptr<Item>> m_itemCache;
};

class Folder : public object::Folder
{
public:
	explicit Folder(const std::shared_ptr<const Store>& store, IMAPIFolder* pFolder, size_t columnCount, mapi_ptr<SPropValue>&& columns);
	~Folder() override;

	// Accessors used by other MAPIGraphQL classes
	enum class DefaultColumn : size_t {
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
		constexpr std::array folderProps{
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
		return {
			SSortOrder { PR_DISPLAY_NAME_W, TABLE_SORT_ASCEND }
		};
	}

	const response::IdType& instanceKey() const;
	const response::IdType& id() const;
	const response::StringType& name() const;
	response::IntType count() const;
	response::IntType unread() const;
	const CComPtr<IMAPIFolder>& folder() const;
	const std::vector<std::shared_ptr<Folder>>& subFolders() const;
	std::shared_ptr<Folder> parentFolder() const;
	std::shared_ptr<Folder> lookupSubFolder(const response::IdType& id) const;
	const std::vector<std::shared_ptr<Item>>& items() const;
	std::shared_ptr<Item> lookupItem(const response::IdType& id) const;

private:
	// Resolvers/Accessors which implement the GraphQL type
	service::FieldResult<response::IdType> getId(service::FieldParams&& params) const override;
	service::FieldResult<std::shared_ptr<object::Folder>> getParentFolder(service::FieldParams&& params) const override;
	service::FieldResult<std::shared_ptr<object::Store>> getStore(service::FieldParams&& params) const override;
	service::FieldResult<response::StringType> getName(service::FieldParams&& params) const override;
	service::FieldResult<response::IntType> getCount(service::FieldParams&& params) const override;
	service::FieldResult<response::IntType> getUnread(service::FieldParams&& params) const override;
	service::FieldResult<std::optional<SpecialFolder>> getSpecialFolder(service::FieldParams&& params) const override;
	service::FieldResult<std::vector<std::shared_ptr<object::Property>>> getColumns(service::FieldParams&& params) const override;
	service::FieldResult<std::vector<std::shared_ptr<object::Folder>>> getSubFolders(service::FieldParams&& params, std::optional<std::vector<response::IdType>>&& idsArg) const override;
	service::FieldResult<std::vector<std::shared_ptr<object::Conversation>>> getConversations(service::FieldParams&& params, std::optional<std::vector<response::IdType>>&& idsArg) const override;
	service::FieldResult<std::vector<std::shared_ptr<object::Item>>> getItems(service::FieldParams&& params, std::optional<std::vector<response::IdType>>&& idsArg) const override;

	// Used during construction
	const SPropValue& GetColumnProp(DefaultColumn column) const;
	response::IdType GetIdColumn(DefaultColumn column) const;
	response::StringType GetStringColumn(DefaultColumn column) const;
	response::IntType GetIntColumn(DefaultColumn column) const;

	// These are all initialized at construction.
	const std::weak_ptr<const Store> m_store;
	const size_t m_columnCount;
	const mapi_ptr<SPropValue> m_columns;
	const response::IdType m_instanceKey;
	const response::IdType m_id;
	const response::IdType m_parentId;
	const response::StringType m_name;
	const response::IntType m_count;
	const response::IntType m_unread;
	const std::optional<SpecialFolder> m_specialFolder;

	// These lazy load and cache results between calls to const methods.
	void OpenFolder() const;
	void LoadSubFolders(response::Value&& fieldDirectives) const;
	void LoadItems(response::Value&& fieldDirectives) const;

	mutable CComPtr<IMAPIFolder> m_folder;
	mutable std::unique_ptr<std::map<response::IdType, size_t>> m_subFolderIds;
	mutable std::unique_ptr<std::vector<std::shared_ptr<Folder>>> m_subFolders;
	mutable CComPtr<AdviseSinkProxy<IMAPITable>> m_subFolderSink;
	mutable response::Value m_subFolderDirectives;
	mutable std::unique_ptr<std::map<response::IdType, size_t>> m_itemIds;
	mutable std::unique_ptr<std::vector<std::shared_ptr<Item>>> m_items;
	mutable CComPtr<AdviseSinkProxy<IMAPITable>> m_itemSink;
	mutable response::Value m_itemDirectives;
};

class Item : public object::Item
{
public:
	explicit Item(const std::shared_ptr<const Store>& store, IMessage* pMessage, size_t columnCount, mapi_ptr<SPropValue>&& columns);

	// Accessors used by other MAPIGraphQL classes
	enum class DefaultColumn : size_t {
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
		return {
			SSortOrder { PR_MESSAGE_DELIVERY_TIME, TABLE_SORT_DESCEND }
		};
	}

	const response::IdType& instanceKey() const;
	const response::IdType& id() const;
	const response::StringType& subject() const;
	const std::optional<response::StringType>& sender() const;
	const std::optional<response::StringType>& to() const;
	const std::optional<response::StringType>& cc() const;
	response::BooleanType read() const;
	const FILETIME& received() const;
	const FILETIME& modified() const;
	const std::optional<response::StringType>& preview() const;
	const CComPtr<IMessage>& message() const;

private:
	// Resolvers/Accessors which implement the GraphQL type
	service::FieldResult<response::IdType> getId(service::FieldParams&& params) const override;
	service::FieldResult<std::shared_ptr<object::Folder>> getParentFolder(service::FieldParams&& params) const override;
	service::FieldResult<std::shared_ptr<object::Conversation>> getConversation(service::FieldParams&& params) const override;
	service::FieldResult<response::StringType> getSubject(service::FieldParams&& params) const override;
	service::FieldResult<std::optional<response::StringType>> getSender(service::FieldParams&& params) const override;
	service::FieldResult<std::optional<response::StringType>> getTo(service::FieldParams&& params) const override;
	service::FieldResult<std::optional<response::StringType>> getCc(service::FieldParams&& params) const override;
	service::FieldResult<std::optional<response::Value>> getBody(service::FieldParams&& params) const override;
	service::FieldResult<response::BooleanType> getRead(service::FieldParams&& params) const override;
	service::FieldResult<std::optional<response::Value>> getReceived(service::FieldParams&& params) const override;
	service::FieldResult<std::optional<response::Value>> getModified(service::FieldParams&& params) const override;
	service::FieldResult<std::optional<response::StringType>> getPreview(service::FieldParams&& params) const override;
	service::FieldResult<std::vector<std::shared_ptr<object::Property>>> getColumns(service::FieldParams&& params) const override;
	service::FieldResult<std::vector<std::shared_ptr<service::Object>>> getAttachments(service::FieldParams&& params, std::optional<std::vector<response::IdType>>&& idsArg) const override;

	// Used during construction
	const SPropValue& GetColumnProp(DefaultColumn column) const;
	response::IdType GetIdColumn(DefaultColumn column) const;
	response::StringType GetStringColumn(DefaultColumn column) const;
	response::BooleanType GetReadColumn(DefaultColumn column) const;
	FILETIME GetTimeColumn(DefaultColumn column) const;

	// These are all initialized at construction.
	const std::weak_ptr<const Store> m_store;
	const size_t m_columnCount;
	const mapi_ptr<SPropValue> m_columns;
	const response::IdType m_instanceKey;
	const response::IdType m_id;
	const response::IdType m_parentId;
	const response::StringType m_subject;
	const std::optional<response::StringType> m_sender;
	const std::optional<response::StringType> m_to;
	const std::optional<response::StringType> m_cc;
	const response::BooleanType m_read;
	const FILETIME m_received;
	const FILETIME m_modified;
	const std::optional<response::StringType> m_preview;

	// These lazy load and cache results between calls to const methods.
	void OpenItem() const;

	mutable CComPtr<IMessage> m_message;
};

class Property : public object::Property
{
public:
	using id_variant = std::variant<ULONG, MAPINAMEID>;

	explicit Property(const id_variant& id, mapi_ptr<SPropValue>&& value);

private:
	// Resolvers/Accessors which implement the GraphQL type
	service::FieldResult<std::shared_ptr<service::Object>> getId(service::FieldParams&& params) const override;
	service::FieldResult<std::shared_ptr<service::Object>> getValue(service::FieldParams&& params) const override;

	const std::shared_ptr<service::Object> m_id;
	const mapi_ptr<SPropValue> m_value;
};

class IntId : public object::IntId
{
public:
	explicit IntId(ULONG id);

private:
	// Resolvers/Accessors which implement the GraphQL type
	service::FieldResult<response::IntType> getId(service::FieldParams&& params) const override;

	const ULONG m_id;
};

class NamedId : public object::NamedId
{
public:
	explicit NamedId(const MAPINAMEID& named);

private:
	// Resolvers/Accessors which implement the GraphQL type
	service::FieldResult<response::Value> getPropset(service::FieldParams&& params) const override;
	service::FieldResult<std::shared_ptr<service::Object>> getId(service::FieldParams&& params) const override;

	const GUID m_propset;
	const std::shared_ptr<service::Object> m_named;
};

class StringId : public object::StringId
{
public:
	explicit StringId(PCWSTR name);

private:
	// Resolvers/Accessors which implement the GraphQL type
	service::FieldResult<response::StringType> getName(service::FieldParams&& params) const override;

	const std::wstring m_name;
};

class IntValue : public object::IntValue
{
public:
	explicit IntValue(response::IntType value);

private:
	// Resolvers/Accessors which implement the GraphQL type
	service::FieldResult<response::IntType> getValue(service::FieldParams&& params) const override;

	const response::IntType m_value;
};

class BoolValue : public object::BoolValue
{
public:
	explicit BoolValue(response::BooleanType value);

private:
	// Resolvers/Accessors which implement the GraphQL type
	service::FieldResult<response::BooleanType> getValue(service::FieldParams&& params) const override;

	const response::BooleanType m_value;
};

class StringValue : public object::StringValue
{
public:
	explicit StringValue(PCWSTR value);
	explicit StringValue(std::string&& value);

private:
	// Resolvers/Accessors which implement the GraphQL type
	service::FieldResult<response::StringType> getValue(service::FieldParams&& params) const override;

	const response::StringType m_value;
};

class GuidValue : public object::GuidValue
{
public:
	explicit GuidValue(const GUID& value);

private:
	// Resolvers/Accessors which implement the GraphQL type
	service::FieldResult<response::Value> getValue(service::FieldParams&& params) const override;

	const GUID m_value;
};

class DateTimeValue : public object::DateTimeValue
{
public:
	explicit DateTimeValue(const FILETIME& value);

private:
	// Resolvers/Accessors which implement the GraphQL type
	service::FieldResult<response::Value> getValue(service::FieldParams&& params) const override;

	const FILETIME m_value;
};

class BinaryValue : public object::BinaryValue
{
public:
	explicit BinaryValue(const SBinary& value);

private:
	// Resolvers/Accessors which implement the GraphQL type
	service::FieldResult<response::IdType> getValue(service::FieldParams&& params) const override;

	const response::IdType m_value;
};

class ItemAdded : public object::ItemAdded
{
public:
	explicit ItemAdded(response::IntType index, const std::shared_ptr<Item>& added);

private:
	// Resolvers/Accessors which implement the GraphQL type
	service::FieldResult<response::IntType> getIndex(service::FieldParams&& params) const override;
	service::FieldResult<std::shared_ptr<object::Item>> getAdded(service::FieldParams&& params) const override;

	const response::IntType m_index;
	const std::shared_ptr<Item> m_added;
};

class ItemUpdated : public object::ItemUpdated
{
public:
	explicit ItemUpdated(response::IntType index, const std::shared_ptr<Item>& updated);

private:
	// Resolvers/Accessors which implement the GraphQL type
	service::FieldResult<response::IntType> getIndex(service::FieldParams&& params) const override;
	service::FieldResult<std::shared_ptr<object::Item>> getUpdated(service::FieldParams&& params) const override;

	const response::IntType m_index;
	const std::shared_ptr<Item> m_updated;
};

class ItemRemoved : public object::ItemRemoved
{
public:
	explicit ItemRemoved(response::IntType index, const response::IdType& removed);

private:
	// Resolvers/Accessors which implement the GraphQL type
	service::FieldResult<response::IntType> getIndex(service::FieldParams&& params) const override;
	service::FieldResult<response::IdType> getRemoved(service::FieldParams&& params) const override;

	const response::IntType m_index;
	const response::IdType m_removed;
};

class ItemsReloaded : public object::ItemsReloaded
{
public:
	explicit ItemsReloaded(const std::vector<std::shared_ptr<Item>>& reloaded);

private:
	// Resolvers/Accessors which implement the GraphQL type
	service::FieldResult<std::vector<std::shared_ptr<object::Item>>> getReloaded(service::FieldParams&& params) const override;

	const std::vector<std::shared_ptr<Item>> m_reloaded;
};

class ItemsSubscription : public object::Subscription
{
public:
	explicit ItemsSubscription(std::vector<std::shared_ptr<service::Object>>&& items);

private:
	// Resolvers/Accessors which implement the GraphQL type
	service::FieldResult<std::vector<std::shared_ptr<service::Object>>> getItems(service::FieldParams&& params, ObjectId&& folderIdArg) const override;
	service::FieldResult<std::vector<std::shared_ptr<service::Object>>> getSubFolders(service::FieldParams&& params, ObjectId&& parentFolderIdArg) const override;
	service::FieldResult<std::vector<std::shared_ptr<service::Object>>> getRootFolders(service::FieldParams&& params, response::IdType&& storeIdArg) const override;

	// These are all initialized at construction.
	std::vector<std::shared_ptr<service::Object>> m_items;
};

class FolderAdded : public object::FolderAdded
{
public:
	explicit FolderAdded(response::IntType index, const std::shared_ptr<Folder>& added);

private:
	// Resolvers/Accessors which implement the GraphQL type
	service::FieldResult<response::IntType> getIndex(service::FieldParams&& params) const override;
	service::FieldResult<std::shared_ptr<object::Folder>> getAdded(service::FieldParams&& params) const override;

	const response::IntType m_index;
	const std::shared_ptr<Folder> m_added;
};

class FolderUpdated : public object::FolderUpdated
{
public:
	explicit FolderUpdated(response::IntType index, const std::shared_ptr<Folder>& updated);

private:
	// Resolvers/Accessors which implement the GraphQL type
	service::FieldResult<response::IntType> getIndex(service::FieldParams&& params) const override;
	service::FieldResult<std::shared_ptr<object::Folder>> getUpdated(service::FieldParams&& params) const override;

	const response::IntType m_index;
	const std::shared_ptr<Folder> m_updated;
};

class FolderRemoved : public object::FolderRemoved
{
public:
	explicit FolderRemoved(response::IntType index, const response::IdType& removed);

private:
	// Resolvers/Accessors which implement the GraphQL type
	service::FieldResult<response::IntType> getIndex(service::FieldParams&& params) const override;
	service::FieldResult<response::IdType> getRemoved(service::FieldParams&& params) const override;

	const response::IntType m_index;
	const response::IdType m_removed;
};

class FoldersReloaded : public object::FoldersReloaded
{
public:
	explicit FoldersReloaded(const std::vector<std::shared_ptr<Folder>>& reloaded);

private:
	// Resolvers/Accessors which implement the GraphQL type
	service::FieldResult<std::vector<std::shared_ptr<object::Folder>>> getReloaded(service::FieldParams&& params) const override;

	const std::vector<std::shared_ptr<Folder>> m_reloaded;
};

class SubFoldersSubscription : public object::Subscription
{
public:
	explicit SubFoldersSubscription(std::vector<std::shared_ptr<service::Object>>&& subFolders);

private:
	// Resolvers/Accessors which implement the GraphQL type
	service::FieldResult<std::vector<std::shared_ptr<service::Object>>> getItems(service::FieldParams&& params, ObjectId&& folderIdArg) const override;
	service::FieldResult<std::vector<std::shared_ptr<service::Object>>> getSubFolders(service::FieldParams&& params, ObjectId&& parentFolderIdArg) const override;
	service::FieldResult<std::vector<std::shared_ptr<service::Object>>> getRootFolders(service::FieldParams&& params, response::IdType&& storeIdArg) const override;

	// These are all initialized at construction.
	std::vector<std::shared_ptr<service::Object>> m_subFolders;
};

class RootFoldersSubscription : public object::Subscription
{
public:
	explicit RootFoldersSubscription(std::vector<std::shared_ptr<service::Object>>&& rootFolders);

private:
	// Resolvers/Accessors which implement the GraphQL type
	service::FieldResult<std::vector<std::shared_ptr<service::Object>>> getItems(service::FieldParams&& params, ObjectId&& folderIdArg) const override;
	service::FieldResult<std::vector<std::shared_ptr<service::Object>>> getSubFolders(service::FieldParams&& params, ObjectId&& parentFolderIdArg) const override;
	service::FieldResult<std::vector<std::shared_ptr<service::Object>>> getRootFolders(service::FieldParams&& params, response::IdType&& storeIdArg) const override;

	// These are all initialized at construction.
	std::vector<std::shared_ptr<service::Object>> m_rootFolders;
};

} // namespace graphql::mapi