// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>

#include <graphqlservice/JSONResponse.h>
#include <graphqlservice/internal/Base64.h>

#include "MockObjects.h"

#include "FolderObject.h"
#include "IntIdObject.h"
#include "ItemAddedObject.h"
#include "ItemChangeObject.h"
#include "ItemObject.h"
#include "PropIdObject.h"
#include "PropValueObject.h"
#include "PropertyObject.h"
#include "QueryObject.h"
#include "StoreObject.h"
#include "StringValueObject.h"
#include "SubscriptionObject.h"

using namespace graphql;
using namespace graphql::service;
using namespace graphql::mapi;

using namespace Mock;

using testing::_;
using testing::ByMove;
using testing::Return;
using testing::ReturnRef;

TEST(MAPISchemaTest, QueryStoreName)
{
	auto mockQuery = std::make_shared<MockQuery>();
	auto mockStore = std::make_shared<MockStore>();
	std::string mockName { "mockName" };
	EXPECT_CALL(*mockStore, getName).Times(1).WillOnce(Return(ByMove(mockName)));
	std::vector<std::shared_ptr<object::Store>> stores { std::make_shared<object::Store>(
		mockStore) };
	EXPECT_CALL(*mockQuery, getStores).Times(1).WillOnce(Return(ByMove(stores)));
	auto mockService = std::make_shared<Operations>(std::make_shared<object::Query>(mockQuery),
		std::shared_ptr<object::Mutation> {},
		std::shared_ptr<object::Subscription> {});

	auto ast = R"gql({
		stores {
			name
		}
	})gql"_graphql;
	auto result = response::toJSON(
		mockService->resolve({ ast, {}, response::Value {}, std::launch::async }).get());

	EXPECT_EQ(R"js({"data":{"stores":[{"name":"mockName"}]}})js", result)
		<< "should get the mock store name";
}

TEST(MAPISchemaTest, QueryStoreEmail)
{
	constexpr int propertyId = 3;
	auto mockQuery = std::make_shared<MockQuery>();
	auto mockStore = std::make_shared<MockStore>();
	std::vector<std::shared_ptr<object::Store>> stores { std::make_shared<object::Store>(
		mockStore) };
	EXPECT_CALL(*mockQuery, getStores(MatchPropIdInts(std::vector<int> { { propertyId } }), _))
		.Times(1)
		.WillOnce(Return(ByMove(stores)));
	auto mockProperty = std::make_shared<MockProperty>();
	std::vector<std::shared_ptr<object::Property>> columns { std::make_shared<object::Property>(
		mockProperty) };
	EXPECT_CALL(*mockStore, getColumns)
		.Times(1)
		.WillOnce(
			Return(ByMove(std::vector<std::shared_ptr<object::Property>> { std::move(columns) })));
	auto mockIntId = std::make_shared<MockIntId>();
	auto mockPropId = std::make_shared<object::PropId>(std::make_shared<object::IntId>(mockIntId));
	EXPECT_CALL(*mockProperty, getId).Times(1).WillOnce(Return(ByMove(mockPropId)));
	EXPECT_CALL(*mockIntId, getId).Times(1).WillOnce(Return(propertyId));
	auto mockStringValue = std::make_shared<MockStringValue>();
	EXPECT_CALL(*mockProperty, getValue)
		.Times(1)
		.WillOnce(Return(std::make_shared<object::PropValue>(
			std::make_shared<object::StringValue>(mockStringValue))));
	std::string mockValue { "mockEmail" };
	EXPECT_CALL(*mockStringValue, getValue).Times(1).WillOnce(Return(ByMove(mockValue)));
	auto mockService = std::make_shared<Operations>(std::make_shared<object::Query>(mockQuery),
		std::shared_ptr<object::Mutation> {},
		std::shared_ptr<object::Subscription> {});

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
		mockService->resolve({ ast, {}, response::Value {}, std::launch::async }).get());

	EXPECT_EQ(
		R"js({"data":{"stores":[{"columns":[{"id":{"__typename":"IntId","id":3},"value":{"__typename":"StringValue","value":"mockEmail"}}]}]}})js",
		result)
		<< "should get the mock store email address";
}

TEST(MAPISchemaTest, QueryInboxFolderName)
{
	constexpr int propertyId = 3;
	auto mockQuery = std::make_shared<MockQuery>();
	auto mockStore = std::make_shared<MockStore>();
	std::vector<std::shared_ptr<object::Store>> stores { std::make_shared<object::Store>(
		mockStore) };
	EXPECT_CALL(*mockQuery, getStores).Times(1).WillOnce(Return(ByMove(stores)));
	auto mockFolder = std::make_shared<MockFolder>();
	std::vector<std::shared_ptr<object::Folder>> specialFolders { std::make_shared<object::Folder>(
																	  mockFolder),
		nullptr };
	EXPECT_CALL(*mockStore,
		getSpecialFolders(
			std::vector<SpecialFolder> { SpecialFolder::INBOX, SpecialFolder::ARCHIVE }))
		.Times(1)
		.WillOnce(Return(ByMove(specialFolders)));
	EXPECT_CALL(*mockFolder, getSpecialFolder)
		.Times(1)
		.WillOnce(Return(std::make_optional(SpecialFolder::INBOX)));
	std::string mockName { "Inbox" };
	EXPECT_CALL(*mockFolder, getName).Times(1).WillOnce(Return(ByMove(mockName)));
	auto mockService = std::make_shared<Operations>(std::make_shared<object::Query>(mockQuery),
		std::shared_ptr<object::Mutation> {},
		std::shared_ptr<object::Subscription> {});

	auto ast = R"gql({
		stores {
			specialFolders(ids: [INBOX ARCHIVE]) {
				specialFolder
				name
			}
		}
	})gql"_graphql;
	auto result = response::toJSON(
		mockService->resolve({ ast, {}, response::Value {}, std::launch::async }).get());

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
	auto mockService = std::make_shared<Operations>(std::make_shared<object::Query>(mockQuery),
		std::shared_ptr<object::Mutation> {},
		std::make_shared<object::Subscription>(mockSubscription));
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

	auto mockItemsNotifySubscribe = std::vector { std::make_shared<object::ItemChange>(
		std::make_shared<object::ItemAdded>(mockItemAdded)) };
	EXPECT_CALL(*mockSubscription,
		getItems(MatchResolverContext(ResolverContext::NotifySubscribe), MatchObjectId(folderId)))
		.Times(1)
		.WillOnce(Return(ByMove(mockItemsNotifySubscribe)));
	EXPECT_CALL(*mockItemAdded, getIndex(MatchResolverContext(ResolverContext::NotifySubscribe)))
		.Times(1)
		.WillOnce(Return(int { 5 }));
	auto mockAddedItem = std::make_shared<object::Item>(mockAdded);
	EXPECT_CALL(*mockItemAdded, getAdded(MatchResolverContext(ResolverContext::NotifySubscribe)))
		.Times(1)
		.WillOnce(Return(ByMove(mockAddedItem)));
	EXPECT_CALL(*mockAdded, getId(MatchResolverContext(ResolverContext::NotifySubscribe)))
		.Times(1)
		.WillOnce(ReturnRef(folderId.objectId));
	auto key = mockService
				   ->subscribe({ [&payload](response::Value document) {
									payload = std::move(document);
								},
					   std::move(query),
					   {},
					   response::Value(response::Type::Map),
					   std::launch::async })
				   .get();
	auto mockItemsSubscription = std::vector { std::make_shared<object::ItemChange>(
		std::make_shared<object::ItemAdded>(mockItemAdded)) };
	EXPECT_CALL(*mockSubscription,
		getItems(MatchResolverContext(ResolverContext::Subscription), MatchObjectId(folderId)))
		.Times(1)
		.WillOnce(Return(ByMove(mockItemsSubscription)));
	EXPECT_CALL(*mockItemAdded, getIndex(MatchResolverContext(ResolverContext::Subscription)))
		.Times(1)
		.WillOnce(Return(int { 5 }));
	mockAddedItem = std::make_shared<object::Item>(mockAdded);
	EXPECT_CALL(*mockItemAdded, getAdded(MatchResolverContext(ResolverContext::Subscription)))
		.Times(1)
		.WillOnce(Return(ByMove(mockAddedItem)));
	EXPECT_CALL(*mockAdded, getId(MatchResolverContext(ResolverContext::Subscription)))
		.Times(1)
		.WillOnce(ReturnRef(folderId.objectId));
	ASSERT_TRUE(response::Type::Null == payload.type())
		<< "should not set the payload till after the subscription event is delivered";
	mockService->deliver({ "items"sv, {}, std::launch::async });
	auto mockItemsNotifyUnsubscribe = std::vector { std::make_shared<object::ItemChange>(
		std::make_shared<object::ItemAdded>(mockItemAdded)) };
	EXPECT_CALL(*mockSubscription,
		getItems(MatchResolverContext(ResolverContext::NotifyUnsubscribe), MatchObjectId(folderId)))
		.Times(1)
		.WillOnce(Return(ByMove(mockItemsNotifyUnsubscribe)));
	EXPECT_CALL(*mockItemAdded, getIndex(MatchResolverContext(ResolverContext::NotifyUnsubscribe)))
		.Times(1)
		.WillOnce(Return(int { 5 }));
	mockAddedItem = std::make_shared<object::Item>(mockAdded);
	EXPECT_CALL(*mockItemAdded, getAdded(MatchResolverContext(ResolverContext::NotifyUnsubscribe)))
		.Times(1)
		.WillOnce(Return(ByMove(mockAddedItem)));
	EXPECT_CALL(*mockAdded, getId(MatchResolverContext(ResolverContext::NotifyUnsubscribe)))
		.Times(1)
		.WillOnce(ReturnRef(folderId.objectId));
	mockService->unsubscribe({ key, std::launch::async }).get();
	auto result = response::toJSON(std::move(payload));

	std::ostringstream ossExpected;
	ossExpected << R"js({"data":{"items":[{"index":5,"added":{"id":")js" << objectIdEncoded
				<< R"js("}}]}})js";
	EXPECT_EQ(ossExpected.str(), result) << "should get the expected payload";
}
