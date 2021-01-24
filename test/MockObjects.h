// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef MOCKOBJECTS_H
#define MOCKOBJECTS_H

#include <gmock/gmock.h>

#include "MAPIObjects.h"

namespace Mock {

using namespace graphql;
using namespace graphql::service;
using namespace graphql::mapi;

class MockQuery : public object::Query
{
public:
	explicit MockQuery();

	MOCK_METHOD(FieldResult<std::vector<std::shared_ptr<object::Store>>>, getStores,
		(FieldParams && params, std::optional<std::vector<response::IdType>>&& idsArg),
		(const, final));
};

class MockStore : public object::Store
{
public:
	explicit MockStore();

	MOCK_METHOD(FieldResult<response::IdType>, getId, (FieldParams && params), (const, final));
	MOCK_METHOD(
		FieldResult<response::StringType>, getName, (FieldParams && params), (const, final));
	MOCK_METHOD(FieldResult<std::vector<std::shared_ptr<object::Property>>>, getColumns,
		(FieldParams && params), (const, final));
	MOCK_METHOD(FieldResult<std::vector<std::shared_ptr<object::Folder>>>, getRootFolders,
		(FieldParams && params, std::optional<std::vector<response::IdType>>&& idsArg),
		(const, final));
	MOCK_METHOD(FieldResult<std::vector<std::shared_ptr<object::Folder>>>, getSpecialFolders,
		(FieldParams && params, std::vector<SpecialFolder>&& idsArg), (const, final));
	MOCK_METHOD(FieldResult<std::vector<std::shared_ptr<object::Property>>>, getFolderProperties,
		(FieldParams && params, response::IdType&& folderIdArg,
			std::optional<std::vector<Column>>&& idsArg),
		(const, final));
	MOCK_METHOD(FieldResult<std::vector<std::shared_ptr<object::Property>>>, getItemProperties,
		(FieldParams && params, response::IdType&& folderIdArg,
			std::optional<std::vector<Column>>&& idsArg),
		(const, final));
};

MATCHER_P(MatchPropIdInts, propIds, "integer property IDs match")
{
	const auto directive =
		service::ModifiedArgument<response::Value>::require("columns", arg.fieldDirectives);
	const auto columns =
		service::ModifiedArgument<Column>::require<service::TypeModifier::List>("ids", directive);
	size_t count = 0;
	bool failed = false;
	std::vector<response::IntType> argIds(columns.size());

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

class MockProperty : public object::Property
{
public:
	explicit MockProperty();

	MOCK_METHOD(
		FieldResult<std::shared_ptr<Object>>, getId, (FieldParams && params), (const, final));
	MOCK_METHOD(
		FieldResult<std::shared_ptr<Object>>, getValue, (FieldParams && params), (const, final));
};

class MockIntId : public object::IntId
{
public:
	explicit MockIntId();

	MOCK_METHOD(FieldResult<response::IntType>, getId, (FieldParams && params), (const, final));
};

class MockStringValue : public object::StringValue
{
public:
	explicit MockStringValue();

	MOCK_METHOD(
		FieldResult<response::StringType>, getValue, (FieldParams && params), (const, final));
};

class MockFolder : public object::Folder
{
public:
	explicit MockFolder();

	MOCK_METHOD(FieldResult<response::IdType>, getId, (FieldParams && params), (const, final));
	MOCK_METHOD(FieldResult<std::shared_ptr<object::Folder>>, getParentFolder,
		(FieldParams && params), (const, final));
	MOCK_METHOD(FieldResult<std::shared_ptr<object::Store>>, getStore, (FieldParams && params),
		(const, final));
	MOCK_METHOD(
		FieldResult<response::StringType>, getName, (FieldParams && params), (const, final));
	MOCK_METHOD(FieldResult<response::IntType>, getCount, (FieldParams && params), (const, final));
	MOCK_METHOD(FieldResult<response::IntType>, getUnread, (FieldParams && params), (const, final));
	MOCK_METHOD(FieldResult<std::optional<SpecialFolder>>, getSpecialFolder,
		(FieldParams && params), (const, final));
	MOCK_METHOD(FieldResult<std::vector<std::shared_ptr<object::Property>>>, getColumns,
		(FieldParams && params), (const, final));
	MOCK_METHOD(FieldResult<std::vector<std::shared_ptr<object::Folder>>>, getSubFolders,
		(FieldParams && params, std::optional<std::vector<response::IdType>>&& idsArg),
		(const, final));
	MOCK_METHOD(FieldResult<std::vector<std::shared_ptr<object::Conversation>>>, getConversations,
		(FieldParams && params, std::optional<std::vector<response::IdType>>&& idsArg),
		(const, final));
	MOCK_METHOD(FieldResult<std::vector<std::shared_ptr<object::Item>>>, getItems,
		(FieldParams && params, std::optional<std::vector<response::IdType>>&& idsArg),
		(const, final));
};

class MockConversation : public object::Conversation
{
public:
	explicit MockConversation();

	MOCK_METHOD(FieldResult<response::IdType>, getId, (FieldParams && params), (const, final));
	MOCK_METHOD(
		FieldResult<response::StringType>, getSubject, (FieldParams && params), (const, final));
	MOCK_METHOD(FieldResult<response::IntType>, getCount, (FieldParams && params), (const, final));
	MOCK_METHOD(FieldResult<response::IntType>, getUnread, (FieldParams && params), (const, final));
	MOCK_METHOD(FieldResult<std::optional<response::Value>>, getReceived, (FieldParams && params),
		(const, final));
	MOCK_METHOD(FieldResult<std::vector<std::shared_ptr<object::Item>>>, getItems,
		(FieldParams && params, std::optional<std::vector<response::IdType>>&& idsArg),
		(const, final));
};

class MockItem : public object::Item
{
public:
	explicit MockItem();

	MOCK_METHOD(FieldResult<response::IdType>, getId, (FieldParams && params), (const, final));
	MOCK_METHOD(FieldResult<std::shared_ptr<object::Folder>>, getParentFolder,
		(FieldParams && params), (const, final));
	MOCK_METHOD(FieldResult<std::shared_ptr<object::Conversation>>, getConversation,
		(FieldParams && params), (const, final));
	MOCK_METHOD(
		FieldResult<response::StringType>, getSubject, (FieldParams && params), (const, final));
	MOCK_METHOD(FieldResult<std::optional<response::StringType>>, getSender,
		(FieldParams && params), (const, final));
	MOCK_METHOD(FieldResult<std::optional<response::StringType>>, getTo, (FieldParams && params), (const, final));
	MOCK_METHOD(FieldResult<std::optional<response::StringType>>, getCc, (FieldParams && params), (const, final));
	MOCK_METHOD(FieldResult<std::optional<response::Value>>, getBody, (FieldParams && params),
		(const, final));
	MOCK_METHOD(
		FieldResult<response::BooleanType>, getRead, (FieldParams && params), (const, final));
	MOCK_METHOD(FieldResult<std::optional<response::Value>>, getReceived, (FieldParams && params),
		(const, final));
	MOCK_METHOD(FieldResult<std::optional<response::Value>>, getModified, (FieldParams && params),
		(const, final));
	MOCK_METHOD(FieldResult<std::optional<response::StringType>>, getPreview,
		(FieldParams && params), (const, final));
	MOCK_METHOD(FieldResult<std::vector<std::shared_ptr<object::Property>>>, getColumns,
		(FieldParams && params), (const, final));
	MOCK_METHOD(FieldResult<std::vector<std::shared_ptr<Object>>>, getAttachments,
		(FieldParams && params, std::optional<std::vector<response::IdType>>&& idsArg),
		(const, final));
};

class MockFileAttachment : public object::FileAttachment
{
public:
	explicit MockFileAttachment();

	MOCK_METHOD(FieldResult<response::IdType>, getId, (FieldParams && params), (const, final));
	MOCK_METHOD(
		FieldResult<response::StringType>, getName, (FieldParams && params), (const, final));
	MOCK_METHOD(FieldResult<std::optional<response::Value>>, getContents, (FieldParams && params),
		(const, final));
	MOCK_METHOD(FieldResult<std::vector<std::shared_ptr<object::Property>>>, getProperties,
		(FieldParams && params, std::optional<std::vector<PropIdInput>>&& idsArg), (const, final));
};

MATCHER_P(MatchResolverContext, resolverContext, "resolverContext matches")
{
	return arg.resolverContext == resolverContext;
}

MATCHER_P(MatchObjectId, objectId, "objectId matches")
{
	return arg.storeId == objectId.storeId && arg.objectId == objectId.objectId;
}

class MockSubscription : public object::Subscription
{
public:
	explicit MockSubscription();

	MOCK_METHOD(FieldResult<std::vector<std::shared_ptr<Object>>>, getItems,
		(FieldParams && params, ObjectId&& folderIdArg), (const, final));
	MOCK_METHOD(FieldResult<std::vector<std::shared_ptr<Object>>>, getSubFolders,
		(FieldParams && params, ObjectId&& parentFolderIdArg), (const, final));
	MOCK_METHOD(FieldResult<std::vector<std::shared_ptr<Object>>>, getRootFolders,
		(FieldParams && params, response::IdType&& storeIdArg), (const, final));
};

class MockItemAdded : public object::ItemAdded
{
public:
	explicit MockItemAdded();

	MOCK_METHOD(FieldResult<response::IntType>, getIndex, (FieldParams && params), (const, final));
	MOCK_METHOD(FieldResult<std::shared_ptr<object::Item>>, getAdded, (FieldParams && params),
		(const, final));
};

class MockItemUpdated : public object::ItemUpdated
{
public:
	explicit MockItemUpdated();

	MOCK_METHOD(FieldResult<response::IntType>, getIndex, (FieldParams && params), (const, final));
	MOCK_METHOD(FieldResult<std::shared_ptr<object::Item>>, getUpdated, (FieldParams && params),
		(const, final));
};

class MockItemRemoved : public object::ItemRemoved
{
public:
	explicit MockItemRemoved();

	MOCK_METHOD(FieldResult<response::IntType>, getIndex, (FieldParams && params), (const, final));
	MOCK_METHOD(FieldResult<response::IdType>, getRemoved, (FieldParams && params), (const, final));
};

class MockItemsReloaded : public object::ItemsReloaded
{
public:
	explicit MockItemsReloaded();

	MOCK_METHOD(FieldResult<std::vector<std::shared_ptr<object::Item>>>, getReloaded,
		(FieldParams && params), (const, final));
};

} // namespace Mock

#endif // MOCKOBJECTS_H
