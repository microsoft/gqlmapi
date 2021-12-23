// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#include "StreamValueObject.h"

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

StreamValue::StreamValue(std::unique_ptr<Concept>&& pimpl) noexcept
	: service::Object{ getTypeNames(), getResolvers() }
	, _pimpl { std::move(pimpl) }
{
}

service::TypeNames StreamValue::getTypeNames() const noexcept
{
	return {
		R"gql(PropValue)gql"sv,
		R"gql(StreamValue)gql"sv
	};
}

service::ResolverMap StreamValue::getResolvers() const noexcept
{
	return {
		{ R"gql(value)gql"sv, [this](service::ResolverParams&& params) { return resolveValue(std::move(params)); } },
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } }
	};
}

void StreamValue::beginSelectionSet(const service::SelectionSetParams& params) const
{
	_pimpl->beginSelectionSet(params);
}

void StreamValue::endSelectionSet(const service::SelectionSetParams& params) const
{
	_pimpl->endSelectionSet(params);
}

service::AwaitableResolver StreamValue::resolveValue(service::ResolverParams&& params) const
{
	std::unique_lock resolverLock(_resolverMutex);
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getValue(service::FieldParams(service::SelectionSetParams{ params }, std::move(directives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::Value>::convert(std::move(result), std::move(params));
}

service::AwaitableResolver StreamValue::resolve_typename(service::ResolverParams&& params) const
{
	return service::ModifiedResult<std::string>::convert(std::string{ R"gql(StreamValue)gql" }, std::move(params));
}

} // namespace object

void AddStreamValueDetails(const std::shared_ptr<schema::ObjectType>& typeStreamValue, const std::shared_ptr<schema::Schema>& schema)
{
	typeStreamValue->AddFields({
		schema::Field::Make(R"gql(value)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(Stream)gql"sv)))
	});
}

} // namespace graphql::mapi
