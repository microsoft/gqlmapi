// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef MOCKOBJECTS_H
#define MOCKOBJECTS_H

#include <gmock/gmock.h>

#include "MAPISchema.h"

namespace Mock {

using namespace graphql;
using namespace graphql::service;
using namespace graphql::mapi;

class MockQuery
{
public:
	explicit MockQuery();

	MOCK_METHOD(std::vector<std::shared_ptr<object::Store>>, getStores,
		(service::FieldParams && params, std::optional<std::vector<response::IdType>>&& idsArg));
};

class MockStore
{
public:
	explicit MockStore();

	MOCK_METHOD(const response::IdType&, getId, (), (const));
	MOCK_METHOD(std::string, getName, (), (const));
	MOCK_METHOD(std::vector<std::shared_ptr<object::Property>>, getColumns, ());
	MOCK_METHOD(std::vector<std::shared_ptr<object::Folder>>, getRootFolders,
		(service::FieldParams && params, std::optional<std::vector<response::IdType>>&& idsArg));
	MOCK_METHOD(std::vector<std::shared_ptr<object::Folder>>, getSpecialFolders,
		(std::vector<SpecialFolder> && idsArg));
	MOCK_METHOD(std::vector<std::shared_ptr<object::Property>>, getFolderProperties,
		(response::IdType && folderIdArg, std::optional<std::vector<Column>>&& idsArg));
	MOCK_METHOD(std::vector<std::shared_ptr<object::Property>>, getItemProperties,
		(response::IdType && itemIdArg, std::optional<std::vector<Column>>&& idsArg));
};

MATCHER_P(MatchPropIdInts, propIds, "integer property IDs match")
{
	const auto itr = std::find_if(arg.fieldDirectives.cbegin(),
		arg.fieldDirectives.cend(),
		[](const auto& entry) noexcept {
			return entry.first == "columns";
		});
	const auto columns =
		service::ModifiedArgument<Column>::require<service::TypeModifier::List>("ids", itr->second);
	size_t count = 0;
	bool failed = false;
	std::vector<int> argIds(columns.size());

	std::transform(columns.cbegin(),
		columns.cend(),
		argIds.begin(),
		[&result_listener, &failed, &count](const Column& input) noexcept {
			if (!input.property.id || input.property.named)
			{
				*result_listener << "invalid property ID at index: " << count++;
				failed = true;
				return 0;
			}

			++count;
			return *input.property.id;
		});

	return !failed && argIds == propIds;
}

class MockProperty
{
public:
	explicit MockProperty();

	MOCK_METHOD(std::shared_ptr<object::PropId>, getId, (), (const));
	MOCK_METHOD(std::shared_ptr<object::PropValue>, getValue, (), (const));
};

class MockIntId
{
public:
	explicit MockIntId();

	MOCK_METHOD(int, getId, (), (const));
};

class MockStringValue
{
public:
	explicit MockStringValue();

	MOCK_METHOD(std::string, getValue, (), (const));
};

class MockFolder
{
public:
	explicit MockFolder();

	MOCK_METHOD(const response::IdType&, getId, (), (const));
	MOCK_METHOD(std::shared_ptr<object::Folder>, getParentFolder, (), (const));
	MOCK_METHOD(std::shared_ptr<object::Store>, getStore, (), (const));
	MOCK_METHOD(std::string, getName, (), (const));
	MOCK_METHOD(int, getCount, (), (const));
	MOCK_METHOD(int, getUnread, (), (const));
	MOCK_METHOD(std::optional<SpecialFolder>, getSpecialFolder, (), (const));
	MOCK_METHOD(std::vector<std::shared_ptr<object::Property>>, getColumns, (), (const));
	MOCK_METHOD(std::vector<std::shared_ptr<object::Folder>>, getSubFolders,
		(service::FieldParams && params, std::optional<std::vector<response::IdType>>&& idsArg));
	MOCK_METHOD(std::vector<std::shared_ptr<object::Conversation>>, getConversations,
		(service::FieldParams && params, std::optional<std::vector<response::IdType>>&& idsArg));
	MOCK_METHOD(std::vector<std::shared_ptr<object::Item>>, getItems,
		(service::FieldParams && params, std::optional<std::vector<response::IdType>>&& idsArg));
};

class MockConversation
{
public:
	explicit MockConversation();

	MOCK_METHOD(const response::IdType&, getId, (), (const));
	MOCK_METHOD(const std::string&, getSubject, (), (const));
	MOCK_METHOD(int, getCount, (), (const));
	MOCK_METHOD(int, getUnread, (), (const));
	MOCK_METHOD(std::optional<response::Value>, getReceived, (), (const));
	MOCK_METHOD(std::vector<std::shared_ptr<object::Item>>, getItems,
		(FieldParams && params, std::optional<std::vector<response::IdType>>&& idsArg));
};

class MockItem
{
public:
	explicit MockItem();

	MOCK_METHOD(const response::IdType&, getId, (service::FieldParams && params), (const));
	MOCK_METHOD(std::shared_ptr<object::Folder>, getParentFolder, (), (const));
	MOCK_METHOD(
		std::shared_ptr<object::Conversation>, getConversation, (FieldParams && params), (const));
	MOCK_METHOD(const std::string&, getSubject, (), (const));
	MOCK_METHOD(std::optional<std::string>, getSender, (), (const));
	MOCK_METHOD(std::optional<std::string>, getTo, (), (const));
	MOCK_METHOD(std::optional<std::string>, getCc, (), (const));
	MOCK_METHOD(std::optional<response::Value>, getBody, (), (const));
	MOCK_METHOD(bool, getRead, (), (const));
	MOCK_METHOD(std::optional<response::Value>, getReceived, (), (const));
	MOCK_METHOD(std::optional<response::Value>, getModified, (), (const));
	MOCK_METHOD(std::optional<std::string>, getPreview, (), (const));
	MOCK_METHOD(std::vector<std::shared_ptr<object::Property>>, getColumns, (), (const));
	MOCK_METHOD(std::vector<std::shared_ptr<object::Attachment>>, getAttachments,
		(FieldParams && params, std::optional<std::vector<response::IdType>>&& idsArg), (const));
};

class MockFileAttachment
{
public:
	explicit MockFileAttachment();

	MOCK_METHOD(const response::IdType&, getId, (), (const));
	MOCK_METHOD(const std::string&, getName, (), (const));
	MOCK_METHOD(std::optional<response::Value>, getContents, (), (const));
	MOCK_METHOD(std::vector<std::shared_ptr<object::Property>>, getProperties,
		(FieldParams && params, std::optional<std::vector<PropIdInput>>&& idsArg), (const));
};

MATCHER_P(MatchResolverContext, resolverContext, "resolverContext matches")
{
	return arg.resolverContext == resolverContext;
}

MATCHER_P(MatchObjectId, objectId, "objectId matches")
{
	return arg.storeId == objectId.storeId && arg.objectId == objectId.objectId;
}

class MockSubscription
{
public:
	explicit MockSubscription();

	MOCK_METHOD(std::vector<std::shared_ptr<object::ItemChange>>, getItems,
		(service::FieldParams && params, ObjectId&& folderIdArg));
	MOCK_METHOD(std::vector<std::shared_ptr<object::FolderChange>>, getSubFolders,
		(service::FieldParams && params, ObjectId&& parentFolderIdArg));
	MOCK_METHOD(std::vector<std::shared_ptr<object::FolderChange>>, getRootFolders,
		(service::FieldParams && params, response::IdType&& storeIdArg));
};

class MockItemAdded
{
public:
	explicit MockItemAdded();

	MOCK_METHOD(int, getIndex, (service::FieldParams && params), (const));
	MOCK_METHOD(std::shared_ptr<object::Item>, getAdded, (service::FieldParams && params), (const));
};

class MockItemUpdated
{
public:
	explicit MockItemUpdated();

	MOCK_METHOD(int, getIndex, (), (const));
	MOCK_METHOD(std::shared_ptr<object::Item>, getUpdated, (), (const));
};

class MockItemRemoved
{
public:
	explicit MockItemRemoved();

	MOCK_METHOD(int, getIndex, (), (const));
	MOCK_METHOD(const response::IdType&, getRemoved, (), (const));
};

class MockItemsReloaded
{
public:
	explicit MockItemsReloaded();

	MOCK_METHOD(std::vector<std::shared_ptr<object::Item>>, getReloaded, (), (const));
};

} // namespace Mock

#endif // MOCKOBJECTS_H
