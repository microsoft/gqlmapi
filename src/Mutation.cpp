// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "DateTime.h"
#include "Guid.h"
#include "Types.h"
#include "Unicode.h"

#include "FolderObject.h"
#include "ItemObject.h"

namespace graphql::mapi {

Mutation::Mutation(const std::shared_ptr<Query>& query)
	: m_query { query }
{
}

Mutation::~Mutation()
{
	// Release all of the MAPI objects we opened and free any MAPI memory allocations.
}

mapi_ptr<ENTRYLIST> ExtractEntryList(const std::vector<response::IdType>& itemIds)
{
	mapi_ptr<ENTRYLIST> entryIds;

	CORt(MAPIAllocateBuffer(
		static_cast<ULONG>(sizeof(*entryIds) + (itemIds.size() * sizeof(*entryIds->lpbin))),
		reinterpret_cast<void**>(&out_ptr { entryIds })));
	CFRt(entryIds != nullptr);
	entryIds->cValues = static_cast<ULONG>(itemIds.size());
	entryIds->lpbin = reinterpret_cast<SBinary*>(entryIds.get() + 1);
	std::transform(itemIds.cbegin(),
		itemIds.cend(),
		entryIds->lpbin,
		[](const auto& itemId) noexcept {
			return SBinary { static_cast<ULONG>(itemId.size()),
				reinterpret_cast<LPBYTE>(const_cast<response::IdType&>(itemId).data()) };
		});

	return entryIds;
}

bool Mutation::CopyItems(MultipleItemsInput&& inputArg, ObjectId&& destinationArg, bool moveItems)
{
	auto storeId = std::move(inputArg.folderId.storeId);
	auto folderId = std::move(inputArg.folderId.objectId);
	auto store = m_query->lookup(storeId);

	CFRt(store != nullptr);

	auto folder = store->OpenFolder(folderId);

	CFRt(folder != nullptr);

	auto targetStore = m_query->lookup(destinationArg.storeId);

	CFRt(targetStore != nullptr);

	auto targetFolder = targetStore->OpenFolder(destinationArg.objectId);

	CFRt(targetFolder != nullptr);

	auto entryIds = ExtractEntryList(inputArg.itemIds);
	HRESULT result = S_OK;

	CORt(result = folder->folder()->CopyMessages(entryIds.get(),
			 &IID_IMAPIFolder,
			 reinterpret_cast<void*>(static_cast<IMAPIFolder*>(targetFolder->folder())),
			 NULL,
			 nullptr,
			 (moveItems ? MESSAGE_MOVE : 0)));

	return (result != MAPI_W_PARTIAL_COMPLETION);
}

void Mutation::endSelectionSet(const service::SelectionSetParams&)
{
	m_query->ClearCaches();
}

std::shared_ptr<object::Item> Mutation::applyCreateItem(CreateItemInput&& inputArg)
{
	auto storeId = std::move(inputArg.folderId.storeId);
	auto folderId = std::move(inputArg.folderId.objectId);
	auto store = m_query->lookup(storeId);

	CFRt(store != nullptr);

	auto parentFolder = store->OpenFolder(folderId);
	CComPtr<IMessage> message;

	CFRt(parentFolder != nullptr);
	CORt(parentFolder->folder()->CreateMessage(nullptr, MAPI_DEFERRED_ERRORS, &message));

	mapi_ptr<SPropValue> properties;
	const size_t count = size_t { 2 } // subject and read
		+ (inputArg.conversationId ? size_t { 1 } : size_t { 0 })
		+ (inputArg.received ? size_t { 1 } : size_t { 0 })
		+ (inputArg.modified ? size_t { 1 } : size_t { 0 })
		+ (inputArg.properties ? inputArg.properties->size() : size_t { 0 });
	const size_t propertiesSize = count * sizeof(*properties);
	size_t nextProp = 0;

	CORt(::MAPIAllocateBuffer(static_cast<ULONG>(propertiesSize),
		reinterpret_cast<void**>(&out_ptr { properties })));
	CFRt(properties != nullptr);
	CFRt(nextProp < count);

	auto subject = convert::utf8::to_utf16(inputArg.subject);
	const size_t subjectSize = (subject.size() + 1) * sizeof(wchar_t);
	auto& subjectProp = properties.get()[nextProp++];

	CORt(::MAPIAllocateMore(static_cast<ULONG>(subjectSize),
		properties.get(),
		reinterpret_cast<void**>(&subjectProp.Value.lpszW)));
	CFRt(subjectProp.Value.lpszW != nullptr);
	subjectProp.ulPropTag = PR_SUBJECT_W;
	memmove(subjectProp.Value.lpszW, subject.data(), subjectSize);
	subjectProp.Value.lpszW[subject.size()] = L'\0';
	CFRt(nextProp < count);

	auto& readProp = properties.get()[nextProp++];

	readProp.ulPropTag = PR_MESSAGE_FLAGS;
	readProp.Value.l = inputArg.read ? MSGFLAG_READ : 0;

	if (inputArg.conversationId)
	{
		CFRt(nextProp < count);

		auto& conversationProp = properties.get()[nextProp++];

		conversationProp.ulPropTag = PR_CONVERSATION_ID;
		conversationProp.Value.bin.cb = static_cast<ULONG>(inputArg.conversationId->size());
		conversationProp.Value.bin.lpb = reinterpret_cast<LPBYTE>(inputArg.conversationId->data());
	}

	if (inputArg.received)
	{
		CFRt(nextProp < count);

		auto& receivedProp = properties.get()[nextProp++];

		receivedProp.ulPropTag = PR_MESSAGE_DELIVERY_TIME;
		receivedProp.Value.ft =
			convert::datetime::from_string(inputArg.received->get<std::string>());
	}

	if (inputArg.modified)
	{
		CFRt(nextProp < count);

		auto& modifiedProp = properties.get()[nextProp++];

		modifiedProp.ulPropTag = PR_LAST_MODIFICATION_TIME;
		modifiedProp.Value.ft =
			convert::datetime::from_string(inputArg.modified->get<std::string>());
	}

	if (inputArg.properties)
	{
		const LPSPropValue propBegin = properties.get() + nextProp;
		const LPSPropValue propEnd = properties.get() + count;

		nextProp += std::distance(propBegin, propEnd);
		store->ConvertPropertyInputs(properties.get(),
			propBegin,
			propEnd,
			std::move(*inputArg.properties));
	}

	CFRt(nextProp == count);
	CORt(message->SetProps(static_cast<ULONG>(count), properties.get(), nullptr));
	CORt(message->SaveChanges(0));

	auto created = std::make_shared<Item>(store, message, count, std::move(properties));

	store->CacheItem(created);
	return std::make_shared<object::Item>(created);
}

std::shared_ptr<object::Folder> Mutation::applyCreateSubFolder(CreateSubFolderInput&& inputArg)
{
	auto storeId = std::move(inputArg.folderId.storeId);
	auto folderId = std::move(inputArg.folderId.objectId);
	auto store = m_query->lookup(storeId);

	CFRt(store != nullptr);

	auto parentFolder = store->OpenFolder(folderId);
	CComPtr<IMAPIFolder> folder;

	CFRt(parentFolder != nullptr);
	CORt(parentFolder->folder()->CreateFolder(FOLDER_GENERIC,
		const_cast<char*>(""),
		const_cast<char*>(""),
		nullptr,
		MAPI_DEFERRED_ERRORS | MAPI_UNICODE,
		&folder));

	mapi_ptr<SPropValue> properties;
	const size_t count = 1 // name
		+ (inputArg.properties ? inputArg.properties->size() : 0);
	const size_t propertiesSize = count * sizeof(*properties);
	size_t nextProp = 0;

	CORt(::MAPIAllocateBuffer(static_cast<ULONG>(propertiesSize),
		reinterpret_cast<void**>(&out_ptr { properties })));
	CFRt(properties != nullptr);
	CFRt(nextProp < count);

	auto name = convert::utf8::to_utf16(inputArg.name);
	const size_t nameSize = (name.size() + 1) * sizeof(wchar_t);
	auto& nameProp = properties.get()[nextProp++];

	CORt(::MAPIAllocateMore(static_cast<ULONG>(nameSize),
		properties.get(),
		reinterpret_cast<void**>(&nameProp.Value.lpszW)));
	CFRt(nameProp.Value.lpszW != nullptr);
	nameProp.ulPropTag = PR_DISPLAY_NAME_W;
	memmove(nameProp.Value.lpszW, name.data(), nameSize);
	nameProp.Value.lpszW[name.size()] = L'\0';
	CFRt(nextProp < count);

	if (inputArg.properties)
	{
		const LPSPropValue propBegin = properties.get() + nextProp;
		const LPSPropValue propEnd = properties.get() + count;

		nextProp += std::distance(propBegin, propEnd);
		store->ConvertPropertyInputs(properties.get(),
			propBegin,
			propEnd,
			std::move(*inputArg.properties));
	}

	CORt(folder->SetProps(static_cast<ULONG>(count), properties.get(), nullptr));
	CORt(folder->SaveChanges(0));

	auto created = std::make_shared<Folder>(store, folder, count, std::move(properties));

	store->CacheFolder(created);
	return std::make_shared<object::Folder>(created);
}

std::shared_ptr<object::Item> Mutation::applyModifyItem(ModifyItemInput&& inputArg)
{
	auto storeId = std::move(inputArg.id.storeId);
	auto messageId = std::move(inputArg.id.objectId);
	auto store = m_query->lookup(storeId);

	CFRt(store != nullptr);

	auto message = store->OpenItem(messageId);

	CFRt(message != nullptr);

	const size_t deleteCount = (inputArg.deleted ? inputArg.deleted->size() : 0);

	if (deleteCount > 0)
	{
		auto propIds = store->lookupPropIdInputs(std::move(*inputArg.deleted));
		mapi_ptr<SPropTagArray> deletePropIds;

		CORt(::MAPIAllocateBuffer(CbNewSPropTagArray(propIds.size()),
			reinterpret_cast<void**>(&out_ptr { deletePropIds })));
		CFRt(deletePropIds != nullptr);
		deletePropIds->cValues = static_cast<ULONG>(propIds.size());
		std::transform(propIds.cbegin(),
			propIds.cend(),
			deletePropIds->aulPropTag,
			[](const auto& entry) noexcept {
				return entry.first;
			});

		CORt(message->message()->DeleteProps(deletePropIds.get(), nullptr));
	}

	const size_t setCount =
		(inputArg.subject ? 1 : 0) + (inputArg.properties ? inputArg.properties->size() : 0);

	if (setCount > 0)
	{
		mapi_ptr<SPropValue> properties;
		const size_t propertiesSize = setCount * sizeof(*properties);
		size_t nextProp = 0;

		CORt(::MAPIAllocateBuffer(static_cast<ULONG>(propertiesSize),
			reinterpret_cast<void**>(&out_ptr { properties })));
		CFRt(properties != nullptr);
		CFRt(nextProp < setCount);

		if (inputArg.subject)
		{
			auto subject = convert::utf8::to_utf16(*inputArg.subject);
			const size_t subjectSize = (subject.size() + 1) * sizeof(wchar_t);
			auto& subjectProp = properties.get()[nextProp++];

			CORt(::MAPIAllocateMore(static_cast<ULONG>(subjectSize),
				properties.get(),
				reinterpret_cast<void**>(&subjectProp.Value.lpszW)));
			CFRt(subjectProp.Value.lpszW != nullptr);
			subjectProp.ulPropTag = PR_SUBJECT_W;
			memmove(subjectProp.Value.lpszW, subject.data(), subjectSize);
			subjectProp.Value.lpszW[subject.size()] = L'\0';
			CFRt(nextProp < setCount);
		}

		if (inputArg.properties)
		{
			const LPSPropValue propBegin = properties.get() + nextProp;
			const LPSPropValue propEnd = properties.get() + setCount;

			nextProp += std::distance(propBegin, propEnd);
			store->ConvertPropertyInputs(properties.get(),
				propBegin,
				propEnd,
				std::move(*inputArg.properties));
		}

		CFRt(nextProp == setCount);
		CORt(message->message()->SetProps(static_cast<ULONG>(setCount), properties.get(), nullptr));
	}

	if (deleteCount > 0 || setCount > 0)
	{
		CORt(message->message()->SaveChanges(MAPI_DEFERRED_ERRORS));
	}

	CORt(message->message()->SetReadFlag(inputArg.read ? 0 : CLEAR_READ_FLAG));

	return std::make_shared<object::Item>(message);
}

std::shared_ptr<object::Folder> Mutation::applyModifyFolder(ModifyFolderInput&& inputArg)
{
	auto storeId = std::move(inputArg.folderId.storeId);
	auto folderId = std::move(inputArg.folderId.objectId);
	auto store = m_query->lookup(storeId);

	CFRt(store != nullptr);

	auto folder = store->OpenFolder(folderId);

	CFRt(folder != nullptr);

	const size_t deleteCount = (inputArg.deleted ? inputArg.deleted->size() : 0);

	if (deleteCount > 0)
	{
		auto propIds = store->lookupPropIdInputs(std::move(*inputArg.deleted));
		mapi_ptr<SPropTagArray> deletePropIds;

		CORt(::MAPIAllocateBuffer(CbNewSPropTagArray(propIds.size()),
			reinterpret_cast<void**>(&out_ptr { deletePropIds })));
		CFRt(deletePropIds != nullptr);
		deletePropIds->cValues = static_cast<ULONG>(propIds.size());
		std::transform(propIds.cbegin(),
			propIds.cend(),
			deletePropIds->aulPropTag,
			[](const auto& entry) noexcept {
				return entry.first;
			});

		CORt(folder->folder()->DeleteProps(deletePropIds.get(), nullptr));
	}

	const size_t setCount =
		(inputArg.name ? 1 : 0) + (inputArg.properties ? inputArg.properties->size() : 0);

	if (setCount > 0)
	{
		mapi_ptr<SPropValue> properties;
		const size_t propertiesSize = setCount * sizeof(*properties);
		size_t nextProp = 0;

		CORt(::MAPIAllocateBuffer(static_cast<ULONG>(propertiesSize),
			reinterpret_cast<void**>(&out_ptr { properties })));
		CFRt(properties != nullptr);
		CFRt(nextProp < setCount);

		if (inputArg.name)
		{
			auto name = convert::utf8::to_utf16(*inputArg.name);
			const size_t nameSize = (name.size() + 1) * sizeof(wchar_t);
			auto& nameProp = properties.get()[nextProp++];

			CORt(::MAPIAllocateMore(static_cast<ULONG>(nameSize),
				properties.get(),
				reinterpret_cast<void**>(&nameProp.Value.lpszW)));
			CFRt(nameProp.Value.lpszW != nullptr);
			nameProp.ulPropTag = PR_DISPLAY_NAME_W;
			memmove(nameProp.Value.lpszW, name.data(), nameSize);
			nameProp.Value.lpszW[name.size()] = L'\0';
			CFRt(nextProp < setCount);
		}

		if (inputArg.properties)
		{
			const LPSPropValue propBegin = properties.get() + nextProp;
			const LPSPropValue propEnd = properties.get() + setCount;

			nextProp += std::distance(propBegin, propEnd);
			store->ConvertPropertyInputs(properties.get(),
				propBegin,
				propEnd,
				std::move(*inputArg.properties));
		}

		CFRt(nextProp == setCount);
		CORt(folder->folder()->SetProps(static_cast<ULONG>(setCount), properties.get(), nullptr));
	}

	if (deleteCount > 0 || setCount > 0)
	{
		CORt(folder->folder()->SaveChanges(0));
	}

	return std::make_shared<object::Folder>(folder);
}

bool Mutation::applyRemoveFolder(ObjectId&& inputArg, bool hardDeleteArg)
{
	auto storeId = std::move(inputArg.storeId);
	auto folderId = std::move(inputArg.objectId);
	auto store = m_query->lookup(storeId);

	CFRt(store != nullptr);

	auto folder = store->OpenFolder(folderId);

	CFRt(folder != nullptr);

	auto parentFolder = folder->parentFolder();

	CFRt(parentFolder != nullptr);

	HRESULT result = S_OK;

	if (hardDeleteArg)
	{
		CORt(result = parentFolder->folder()->DeleteFolder(static_cast<ULONG>(folderId.size()),
				 reinterpret_cast<LPENTRYID>(folderId.data()),
				 NULL,
				 nullptr,
				 DEL_FOLDERS | DEL_MESSAGES));
	}
	else
	{
		auto targetFolder = store->lookupSpecialFolder(SpecialFolder::DELETED);

		CORt(result = parentFolder->folder()->CopyFolder(static_cast<ULONG>(folderId.size()),
				 reinterpret_cast<LPENTRYID>(folderId.data()),
				 &IID_IMAPIFolder,
				 reinterpret_cast<void*>(static_cast<IMAPIFolder*>(targetFolder->folder())),
				 nullptr,
				 NULL,
				 nullptr,
				 FOLDER_MOVE | MAPI_UNICODE));
	}

	return (result != MAPI_W_PARTIAL_COMPLETION);
}

bool Mutation::applyMarkAsRead(MultipleItemsInput&& inputArg, bool readArg)
{
	auto storeId = std::move(inputArg.folderId.storeId);
	auto folderId = std::move(inputArg.folderId.objectId);
	auto store = m_query->lookup(storeId);

	CFRt(store != nullptr);

	auto folder = store->OpenFolder(folderId);

	CFRt(folder != nullptr);

	auto entryIds = ExtractEntryList(inputArg.itemIds);
	HRESULT result = S_OK;

	CORt(result = folder->folder()->SetReadFlags(entryIds.get(),
			 NULL,
			 nullptr,
			 (readArg ? 0 : CLEAR_READ_FLAG)));

	return (result != MAPI_W_PARTIAL_COMPLETION);
}

bool Mutation::applyCopyItems(MultipleItemsInput&& inputArg, ObjectId&& destinationArg)
{
	return CopyItems(std::move(inputArg), std::move(destinationArg), false);
}

bool Mutation::applyMoveItems(MultipleItemsInput&& inputArg, ObjectId&& destinationArg)
{
	return CopyItems(std::move(inputArg), std::move(destinationArg), true);
}

bool Mutation::applyDeleteItems(MultipleItemsInput&& inputArg, bool hardDeleteArg)
{
	if (!hardDeleteArg)
	{
		auto store = m_query->lookup(inputArg.folderId.storeId);

		CFRt(store != nullptr);

		auto targetFolder = store->lookupSpecialFolder(SpecialFolder::DELETED);

		CFRt(targetFolder != nullptr);

		return CopyItems(std::move(inputArg),
			ObjectId { inputArg.folderId.storeId, targetFolder->id() },
			true);
	}

	auto storeId = std::move(inputArg.folderId.storeId);
	auto folderId = std::move(inputArg.folderId.objectId);
	auto store = m_query->lookup(storeId);

	CFRt(store != nullptr);

	auto folder = store->OpenFolder(folderId);

	CFRt(folder != nullptr);

	auto entryIds = ExtractEntryList(inputArg.itemIds);
	HRESULT result = S_OK;

	CORt(result = folder->folder()->DeleteMessages(entryIds.get(), NULL, nullptr, 0));

	return (result != MAPI_W_PARTIAL_COMPLETION);
}

} // namespace graphql::mapi