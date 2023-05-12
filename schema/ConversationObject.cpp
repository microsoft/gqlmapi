// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#include "ConversationObject.h"
#include "ItemObject.h"

#include "graphqlservice/internal/Schema.h"

#include "graphqlservice/introspection/IntrospectionSchema.h"

#include <algorithm>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

using namespace std::literals;

namespace graphql::mapi {
namespace object {

Conversation::Conversation(std::unique_ptr<const Concept> pimpl) noexcept
	: service::Object{ getTypeNames(), getResolvers() }
	, _pimpl { std::move(pimpl) }
{
}

service::TypeNames Conversation::getTypeNames() const noexcept
{
	return {
		R"gql(Conversation)gql"sv
	};
}

service::ResolverMap Conversation::getResolvers() const noexcept
{
	return {
		{ R"gql(id)gql"sv, [this](service::ResolverParams&& params) { return resolveId(std::move(params)); } },
		{ R"gql(count)gql"sv, [this](service::ResolverParams&& params) { return resolveCount(std::move(params)); } },
		{ R"gql(items)gql"sv, [this](service::ResolverParams&& params) { return resolveItems(std::move(params)); } },
		{ R"gql(unread)gql"sv, [this](service::ResolverParams&& params) { return resolveUnread(std::move(params)); } },
		{ R"gql(subject)gql"sv, [this](service::ResolverParams&& params) { return resolveSubject(std::move(params)); } },
		{ R"gql(received)gql"sv, [this](service::ResolverParams&& params) { return resolveReceived(std::move(params)); } },
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } }
	};
}

void Conversation::beginSelectionSet(const service::SelectionSetParams& params) const
{
	_pimpl->beginSelectionSet(params);
}

void Conversation::endSelectionSet(const service::SelectionSetParams& params) const
{
	_pimpl->endSelectionSet(params);
}

service::AwaitableResolver Conversation::resolveId(service::ResolverParams&& params) const
{
	std::unique_lock resolverLock(_resolverMutex);
	service::SelectionSetParams selectionSetParams { static_cast<const service::SelectionSetParams&>(params) };
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getId(service::FieldParams { std::move(selectionSetParams), std::move(directives) });
	resolverLock.unlock();

	return service::ModifiedResult<response::IdType>::convert(std::move(result), std::move(params));
}

service::AwaitableResolver Conversation::resolveSubject(service::ResolverParams&& params) const
{
	std::unique_lock resolverLock(_resolverMutex);
	service::SelectionSetParams selectionSetParams { static_cast<const service::SelectionSetParams&>(params) };
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getSubject(service::FieldParams { std::move(selectionSetParams), std::move(directives) });
	resolverLock.unlock();

	return service::ModifiedResult<std::string>::convert(std::move(result), std::move(params));
}

service::AwaitableResolver Conversation::resolveCount(service::ResolverParams&& params) const
{
	std::unique_lock resolverLock(_resolverMutex);
	service::SelectionSetParams selectionSetParams { static_cast<const service::SelectionSetParams&>(params) };
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getCount(service::FieldParams { std::move(selectionSetParams), std::move(directives) });
	resolverLock.unlock();

	return service::ModifiedResult<int>::convert(std::move(result), std::move(params));
}

service::AwaitableResolver Conversation::resolveUnread(service::ResolverParams&& params) const
{
	std::unique_lock resolverLock(_resolverMutex);
	service::SelectionSetParams selectionSetParams { static_cast<const service::SelectionSetParams&>(params) };
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getUnread(service::FieldParams { std::move(selectionSetParams), std::move(directives) });
	resolverLock.unlock();

	return service::ModifiedResult<int>::convert(std::move(result), std::move(params));
}

service::AwaitableResolver Conversation::resolveReceived(service::ResolverParams&& params) const
{
	std::unique_lock resolverLock(_resolverMutex);
	service::SelectionSetParams selectionSetParams { static_cast<const service::SelectionSetParams&>(params) };
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getReceived(service::FieldParams { std::move(selectionSetParams), std::move(directives) });
	resolverLock.unlock();

	return service::ModifiedResult<response::Value>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::AwaitableResolver Conversation::resolveItems(service::ResolverParams&& params) const
{
	auto argIds = service::ModifiedArgument<response::IdType>::require<service::TypeModifier::Nullable, service::TypeModifier::List>("ids", params.arguments);
	std::unique_lock resolverLock(_resolverMutex);
	service::SelectionSetParams selectionSetParams { static_cast<const service::SelectionSetParams&>(params) };
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getItems(service::FieldParams { std::move(selectionSetParams), std::move(directives) }, std::move(argIds));
	resolverLock.unlock();

	return service::ModifiedResult<Item>::convert<service::TypeModifier::List>(std::move(result), std::move(params));
}

service::AwaitableResolver Conversation::resolve_typename(service::ResolverParams&& params) const
{
	return service::Result<std::string>::convert(std::string{ R"gql(Conversation)gql" }, std::move(params));
}

} // namespace object

void AddConversationDetails(const std::shared_ptr<schema::ObjectType>& typeConversation, const std::shared_ptr<schema::Schema>& schema)
{
	typeConversation->AddFields({
		schema::Field::Make(R"gql(id)gql"sv, R"md(ID of this conversation (distinct from the item or folder IDs))md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(ID)gql"sv))),
		schema::Field::Make(R"gql(subject)gql"sv, R"md(Subject of the conversation, typically common to all of the items)md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(String)gql"sv))),
		schema::Field::Make(R"gql(count)gql"sv, R"md(Total item count)md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(Int)gql"sv))),
		schema::Field::Make(R"gql(unread)gql"sv, R"md(Unread item count)md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(Int)gql"sv))),
		schema::Field::Make(R"gql(received)gql"sv, R"md(Last received/creation time of any item in the conversation)md"sv, std::nullopt, schema->LookupType(R"gql(DateTime)gql"sv)),
		schema::Field::Make(R"gql(items)gql"sv, R"md(List of items in the conversation)md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(Item)gql"sv)))), {
			schema::InputValue::Make(R"gql(ids)gql"sv, R"md(Optional list of item IDs, return all items if `null`)md"sv, schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(ID)gql"sv))), R"gql(null)gql"sv)
		})
	});
}

} // namespace graphql::mapi
