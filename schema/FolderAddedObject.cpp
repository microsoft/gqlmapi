// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#include "FolderAddedObject.h"
#include "FolderObject.h"

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

FolderAdded::FolderAdded(std::unique_ptr<Concept>&& pimpl) noexcept
	: service::Object{ getTypeNames(), getResolvers() }
	, _pimpl { std::move(pimpl) }
{
}

service::TypeNames FolderAdded::getTypeNames() const noexcept
{
	return {
		R"gql(FolderChange)gql"sv,
		R"gql(FolderAdded)gql"sv
	};
}

service::ResolverMap FolderAdded::getResolvers() const noexcept
{
	return {
		{ R"gql(added)gql"sv, [this](service::ResolverParams&& params) { return resolveAdded(std::move(params)); } },
		{ R"gql(index)gql"sv, [this](service::ResolverParams&& params) { return resolveIndex(std::move(params)); } },
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } }
	};
}

void FolderAdded::beginSelectionSet(const service::SelectionSetParams& params) const
{
	_pimpl->beginSelectionSet(params);
}

void FolderAdded::endSelectionSet(const service::SelectionSetParams& params) const
{
	_pimpl->endSelectionSet(params);
}

service::AwaitableResolver FolderAdded::resolveIndex(service::ResolverParams&& params) const
{
	std::unique_lock resolverLock(_resolverMutex);
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getIndex(service::FieldParams(service::SelectionSetParams{ params }, std::move(directives)));
	resolverLock.unlock();

	return service::ModifiedResult<int>::convert(std::move(result), std::move(params));
}

service::AwaitableResolver FolderAdded::resolveAdded(service::ResolverParams&& params) const
{
	std::unique_lock resolverLock(_resolverMutex);
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getAdded(service::FieldParams(service::SelectionSetParams{ params }, std::move(directives)));
	resolverLock.unlock();

	return service::ModifiedResult<Folder>::convert(std::move(result), std::move(params));
}

service::AwaitableResolver FolderAdded::resolve_typename(service::ResolverParams&& params) const
{
	return service::ModifiedResult<std::string>::convert(std::string{ R"gql(FolderAdded)gql" }, std::move(params));
}

} // namespace object

void AddFolderAddedDetails(const std::shared_ptr<schema::ObjectType>& typeFolderAdded, const std::shared_ptr<schema::Schema>& schema)
{
	typeFolderAdded->AddFields({
		schema::Field::Make(R"gql(index)gql"sv, R"md(Index in the subscribed window)md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(Int)gql"sv))),
		schema::Field::Make(R"gql(added)gql"sv, R"md(`Folder` that was added)md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(Folder)gql"sv)))
	});
}

} // namespace graphql::mapi
