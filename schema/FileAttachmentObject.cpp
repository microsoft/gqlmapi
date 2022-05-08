// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#include "FileAttachmentObject.h"
#include "PropertyObject.h"

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

FileAttachment::FileAttachment(std::unique_ptr<const Concept>&& pimpl) noexcept
	: service::Object{ getTypeNames(), getResolvers() }
	, _pimpl { std::move(pimpl) }
{
}

service::TypeNames FileAttachment::getTypeNames() const noexcept
{
	return {
		R"gql(Attachment)gql"sv,
		R"gql(FileAttachment)gql"sv
	};
}

service::ResolverMap FileAttachment::getResolvers() const noexcept
{
	return {
		{ R"gql(id)gql"sv, [this](service::ResolverParams&& params) { return resolveId(std::move(params)); } },
		{ R"gql(name)gql"sv, [this](service::ResolverParams&& params) { return resolveName(std::move(params)); } },
		{ R"gql(contents)gql"sv, [this](service::ResolverParams&& params) { return resolveContents(std::move(params)); } },
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(properties)gql"sv, [this](service::ResolverParams&& params) { return resolveProperties(std::move(params)); } }
	};
}

void FileAttachment::beginSelectionSet(const service::SelectionSetParams& params) const
{
	_pimpl->beginSelectionSet(params);
}

void FileAttachment::endSelectionSet(const service::SelectionSetParams& params) const
{
	_pimpl->endSelectionSet(params);
}

service::AwaitableResolver FileAttachment::resolveId(service::ResolverParams&& params) const
{
	std::unique_lock resolverLock(_resolverMutex);
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getId(service::FieldParams(service::SelectionSetParams{ params }, std::move(directives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::IdType>::convert(std::move(result), std::move(params));
}

service::AwaitableResolver FileAttachment::resolveName(service::ResolverParams&& params) const
{
	std::unique_lock resolverLock(_resolverMutex);
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getName(service::FieldParams(service::SelectionSetParams{ params }, std::move(directives)));
	resolverLock.unlock();

	return service::ModifiedResult<std::string>::convert(std::move(result), std::move(params));
}

service::AwaitableResolver FileAttachment::resolveContents(service::ResolverParams&& params) const
{
	std::unique_lock resolverLock(_resolverMutex);
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getContents(service::FieldParams(service::SelectionSetParams{ params }, std::move(directives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::Value>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::AwaitableResolver FileAttachment::resolveProperties(service::ResolverParams&& params) const
{
	auto argIds = service::ModifiedArgument<mapi::PropIdInput>::require<service::TypeModifier::Nullable, service::TypeModifier::List>("ids", params.arguments);
	std::unique_lock resolverLock(_resolverMutex);
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getProperties(service::FieldParams(service::SelectionSetParams{ params }, std::move(directives)), std::move(argIds));
	resolverLock.unlock();

	return service::ModifiedResult<Property>::convert<service::TypeModifier::List, service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::AwaitableResolver FileAttachment::resolve_typename(service::ResolverParams&& params) const
{
	return service::ModifiedResult<std::string>::convert(std::string{ R"gql(FileAttachment)gql" }, std::move(params));
}

} // namespace object

void AddFileAttachmentDetails(const std::shared_ptr<schema::ObjectType>& typeFileAttachment, const std::shared_ptr<schema::Schema>& schema)
{
	typeFileAttachment->AddFields({
		schema::Field::Make(R"gql(id)gql"sv, R"md(ID of this attachment)md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(ID)gql"sv))),
		schema::Field::Make(R"gql(name)gql"sv, R"md(Display name of this attachment)md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(String)gql"sv))),
		schema::Field::Make(R"gql(contents)gql"sv, R"md(Contents of the file)md"sv, std::nullopt, schema->LookupType(R"gql(Stream)gql"sv)),
		schema::Field::Make(R"gql(properties)gql"sv, R"md(Read properties on the attachment)md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->WrapType(introspection::TypeKind::LIST, schema->LookupType(R"gql(Property)gql"sv))), {
			schema::InputValue::Make(R"gql(ids)gql"sv, R"md(Optional list of property IDs, returns all properties if `null`)md"sv, schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(PropIdInput)gql"sv))), R"gql()gql"sv)
		})
	});
}

} // namespace graphql::mapi
