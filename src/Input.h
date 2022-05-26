// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "MAPISchema.h"

namespace convert::input {

graphql::response::IdType from_input(graphql::response::IdType&& input);
graphql::mapi::ObjectId from_input(graphql::mapi::ObjectId&& input);
graphql::mapi::PropValueInput from_input(graphql::mapi::PropValueInput&& input);
graphql::mapi::CreateItemInput from_input(graphql::mapi::CreateItemInput&& input);
graphql::mapi::MultipleItemsInput from_input(graphql::mapi::MultipleItemsInput&& input);

} // namespace convert::input
