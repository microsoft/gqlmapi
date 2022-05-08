// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#include "AttachmentObject.h"

#include "graphqlservice/internal/Schema.h"

#include "graphqlservice/introspection/IntrospectionSchema.h"

using namespace std::literals;

namespace graphql::mapi {
namespace object {

Attachment::Attachment(std::unique_ptr<const Concept>&& pimpl) noexcept
	: service::Object { pimpl->getTypeNames(), pimpl->getResolvers() }
	, _pimpl { std::move(pimpl) }
{
}

void Attachment::beginSelectionSet(const service::SelectionSetParams& params) const
{
	_pimpl->beginSelectionSet(params);
}

void Attachment::endSelectionSet(const service::SelectionSetParams& params) const
{
	_pimpl->endSelectionSet(params);
}

} // namespace object

void AddAttachmentDetails(const std::shared_ptr<schema::UnionType>& typeAttachment, const std::shared_ptr<schema::Schema>& schema)
{
	typeAttachment->AddPossibleTypes({
		schema->LookupType(R"gql(Item)gql"sv),
		schema->LookupType(R"gql(FileAttachment)gql"sv)
	});
}

} // namespace graphql::mapi
