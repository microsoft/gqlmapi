// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>

#include <graphqlservice/JSONResponse.h>
#include <graphqlservice/internal/Base64.h>

#include "MockObjects.h"

using namespace graphql;
using namespace graphql::service;
using namespace graphql::mapi;

using namespace Mock;

using testing::_;
using testing::ByMove;
using testing::Return;

TEST(MAPISchemaTest, QueryStoreName)
{
	auto mockQuery = std::make_shared<MockQuery>();
	auto mockStore = std::make_shared<MockStore>();
	EXPECT_CALL(*mockStore, getName)
		.Times(1)
		.WillOnce(Return(ByMove(FieldResult<response::StringType> { "mockName" })));
	std::vector<std::shared_ptr<object::Store>> stores { mockStore };
	EXPECT_CALL(*mockQuery, getStores)
		.Times(1)
		.WillOnce(Return(ByMove(
			FieldResult<std::vector<std::shared_ptr<object::Store>>> { std::move(stores) })));
	auto mockService = std::make_shared<Operations>(mockQuery, nullptr, nullptr);

	auto ast = R"gql({
		stores {
			name
		}
	})gql"_graphql;
	auto result = response::toJSON(
		mockService->resolve(std::launch::async, nullptr, ast, "", response::Value {}).get());

	EXPECT_EQ(R"js({"data":{"stores":[{"name":"mockName"}]}})js", result)
		<< "should get the mock store name";
}

TEST(MAPISchemaTest, QueryStoreEmail)
{
	constexpr response::IntType propertyId = 3;
	auto mockQuery = std::make_shared<MockQuery>();
	auto mockStore = std::make_shared<MockStore>();
	std::vector<std::shared_ptr<object::Store>> stores { mockStore };
	EXPECT_CALL(*mockQuery,
		getStores(MatchPropIdInts(std::vector<response::IntType> { { propertyId } }), _))
		.Times(1)
		.WillOnce(Return(ByMove(
			FieldResult<std::vector<std::shared_ptr<object::Store>>> { std::move(stores) })));
	auto mockProperty = std::make_shared<MockProperty>();
	std::vector<std::shared_ptr<object::Property>> columns { mockProperty };
	EXPECT_CALL(*mockStore, getColumns)
		.Times(1)
		.WillOnce(Return(ByMove(
			FieldResult<std::vector<std::shared_ptr<object::Property>>> { std::move(columns) })));
	auto mockIntId = std::make_shared<MockIntId>();
	EXPECT_CALL(*mockProperty, getId)
		.Times(1)
		.WillOnce(Return(ByMove(FieldResult<std::shared_ptr<Object>> { mockIntId })));
	EXPECT_CALL(*mockIntId, getId)
		.Times(1)
		.WillOnce(Return(ByMove(FieldResult<response::IntType> { propertyId })));
	auto mockStringValue = std::make_shared<MockStringValue>();
	EXPECT_CALL(*mockProperty, getValue)
		.Times(1)
		.WillOnce(Return(ByMove(FieldResult<std::shared_ptr<Object>> { mockStringValue })));
	EXPECT_CALL(*mockStringValue, getValue)
		.Times(1)
		.WillOnce(Return(ByMove(FieldResult<response::StringType> { "mockEmail" })));
	auto mockService = std::make_shared<Operations>(mockQuery, nullptr, nullptr);

	auto ast = R"gql({
		stores @columns(ids: [{property: { id: 3 }, type: STRING}]) {
			columns {
				id {
					__typename
					...on IntId {
						id
					}
				}
				value {
					__typename
					...on StringValue {
						value
					}
				}
			}
		}
	})gql"_graphql;
	auto result = response::toJSON(
		mockService->resolve(std::launch::async, nullptr, ast, "", response::Value {}).get());

	EXPECT_EQ(
		R"js({"data":{"stores":[{"columns":[{"id":{"__typename":"IntId","id":3},"value":{"__typename":"StringValue","value":"mockEmail"}}]}]}})js",
		result)
		<< "should get the mock store email address";
}

TEST(MAPISchemaTest, QueryInboxFolderName)
{
	constexpr response::IntType propertyId = 3;
	auto mockQuery = std::make_shared<MockQuery>();
	auto mockStore = std::make_shared<MockStore>();
	std::vector<std::shared_ptr<object::Store>> stores { mockStore };
	EXPECT_CALL(*mockQuery, getStores)
		.Times(1)
		.WillOnce(Return(ByMove(
			FieldResult<std::vector<std::shared_ptr<object::Store>>> { std::move(stores) })));
	auto mockFolder = std::make_shared<MockFolder>();
	std::vector<std::shared_ptr<object::Folder>> specialFolders { mockFolder, nullptr };
	EXPECT_CALL(*mockStore,
		getSpecialFolders(_,
			std::vector<SpecialFolder> { SpecialFolder::INBOX, SpecialFolder::ARCHIVE }))
		.Times(1)
		.WillOnce(Return(ByMove(FieldResult<std::vector<std::shared_ptr<object::Folder>>> {
			std::move(specialFolders) })));
	EXPECT_CALL(*mockFolder, getSpecialFolder)
		.Times(1)
		.WillOnce(Return(ByMove(FieldResult<std::optional<SpecialFolder>> {
			std::make_optional(SpecialFolder::INBOX) })));
	EXPECT_CALL(*mockFolder, getName)
		.Times(1)
		.WillOnce(Return(ByMove(FieldResult<response::StringType> { "Inbox" })));
	auto mockService = std::make_shared<Operations>(mockQuery, nullptr, nullptr);

	auto ast = R"gql({
		stores {
			specialFolders(ids: [INBOX ARCHIVE]) {
				specialFolder
				name
			}
		}
	})gql"_graphql;
	auto result = response::toJSON(
		mockService->resolve(std::launch::async, nullptr, ast, "", response::Value {}).get());

	EXPECT_EQ(
		R"js({"data":{"stores":[{"specialFolders":[{"specialFolder":"INBOX","name":"Inbox"},null]}]}})js",
		result)
		<< "should get Inbox folder name and null for ARCHIVE";
}

namespace {

using namespace std::literals;

constexpr auto storeId = "storeId"sv;
constexpr auto objectId = "objectId"sv;

const ObjectId folderId { { storeId.begin(), storeId.end() },
	{ objectId.begin(), objectId.end() } };
const auto storeIdEncoded = internal::Base64::toBase64(folderId.storeId);
const auto objectIdEncoded = internal::Base64::toBase64(folderId.objectId);

} // namespace

TEST(MAPISchemaTest, NotifySubscribeUnsubscribeItemAdded)
{
	auto mockQuery = std::make_shared<MockQuery>();
	auto mockSubscription = std::make_shared<MockSubscription>();
	auto mockItemAdded = std::make_shared<MockItemAdded>();
	auto mockAdded = std::make_shared<MockItem>();
	auto mockService = std::make_shared<Operations>(mockQuery, nullptr, mockSubscription);
	std::ostringstream ossQuery;
	ossQuery << R"gql(subscription {
		items(folderId: { storeId: ")gql"
			 << storeIdEncoded << R"gql(", objectId: ")gql" << objectIdEncoded << R"gql(" }) {
			...on ItemAdded {
				index
				added {
					id
				}
			}
		}
	})gql";
	auto query = peg::parseString(ossQuery.str());
	response::Value payload;

	EXPECT_CALL(*mockSubscription,
		getItems(MatchResolverContext(ResolverContext::NotifySubscribe), MatchObjectId(folderId)))
		.Times(1)
		.WillOnce(Return(ByMove(FieldResult<std::vector<std::shared_ptr<Object>>> {
			std::vector { std::static_pointer_cast<Object>(mockItemAdded) } })));
	EXPECT_CALL(*mockItemAdded, getIndex(MatchResolverContext(ResolverContext::NotifySubscribe)))
		.Times(1)
		.WillOnce(Return(ByMove(FieldResult<response::IntType> { 5 })));
	EXPECT_CALL(*mockItemAdded, getAdded(MatchResolverContext(ResolverContext::NotifySubscribe)))
		.Times(1)
		.WillOnce(Return(ByMove(FieldResult<std::shared_ptr<object::Item>> {
			std::static_pointer_cast<object::Item>(mockAdded) })));
	EXPECT_CALL(*mockAdded, getId(MatchResolverContext(ResolverContext::NotifySubscribe)))
		.Times(1)
		.WillOnce(Return(ByMove(FieldResult<response::IdType> { folderId.objectId })));
	auto key = mockService
				   ->subscribe(std::launch::async,
					   { nullptr, std::move(query), {}, response::Value(response::Type::Map) },
					   [&payload](std::future<response::Value> documentFuture) {
						   payload = documentFuture.get();
					   })
				   .get();
	EXPECT_CALL(*mockSubscription,
		getItems(MatchResolverContext(ResolverContext::Subscription), MatchObjectId(folderId)))
		.Times(1)
		.WillOnce(Return(ByMove(FieldResult<std::vector<std::shared_ptr<Object>>> {
			std::vector { std::static_pointer_cast<Object>(mockItemAdded) } })));
	EXPECT_CALL(*mockItemAdded, getIndex(MatchResolverContext(ResolverContext::Subscription)))
		.Times(1)
		.WillOnce(Return(ByMove(FieldResult<response::IntType> { 5 })));
	EXPECT_CALL(*mockItemAdded, getAdded(MatchResolverContext(ResolverContext::Subscription)))
		.Times(1)
		.WillOnce(Return(ByMove(FieldResult<std::shared_ptr<object::Item>> {
			std::static_pointer_cast<object::Item>(mockAdded) })));
	EXPECT_CALL(*mockAdded, getId(MatchResolverContext(ResolverContext::Subscription)))
		.Times(1)
		.WillOnce(Return(ByMove(FieldResult<response::IdType> { folderId.objectId })));
	ASSERT_TRUE(response::Type::Null == payload.type())
		<< "should not set the payload till after the subscription event is delivered";
	mockService->deliver(std::launch::async,
		"items",
		[](response::MapType::const_reference) noexcept {
			return true;
		},
		{});
	EXPECT_CALL(*mockSubscription,
		getItems(MatchResolverContext(ResolverContext::NotifyUnsubscribe), MatchObjectId(folderId)))
		.Times(1)
		.WillOnce(Return(ByMove(FieldResult<std::vector<std::shared_ptr<Object>>> {
			std::vector { std::static_pointer_cast<Object>(mockItemAdded) } })));
	EXPECT_CALL(*mockItemAdded, getIndex(MatchResolverContext(ResolverContext::NotifyUnsubscribe)))
		.Times(1)
		.WillOnce(Return(ByMove(FieldResult<response::IntType> { 5 })));
	EXPECT_CALL(*mockItemAdded, getAdded(MatchResolverContext(ResolverContext::NotifyUnsubscribe)))
		.Times(1)
		.WillOnce(Return(ByMove(FieldResult<std::shared_ptr<object::Item>> {
			std::static_pointer_cast<object::Item>(mockAdded) })));
	EXPECT_CALL(*mockAdded, getId(MatchResolverContext(ResolverContext::NotifyUnsubscribe)))
		.Times(1)
		.WillOnce(Return(ByMove(FieldResult<response::IdType> { folderId.objectId })));
	mockService->unsubscribe(std::launch::async, key).get();
	auto result = response::toJSON(std::move(payload));

	std::ostringstream ossExpected;
	ossExpected << R"js({"data":{"items":[{"index":5,"added":{"id":")js" << objectIdEncoded
				<< R"js("}}]}})js";
	EXPECT_EQ(ossExpected.str(), result) << "should get the expected payload";
}
