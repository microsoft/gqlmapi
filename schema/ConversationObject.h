// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#pragma once

#ifndef CONVERSATIONOBJECT_H
#define CONVERSATIONOBJECT_H

#include "MAPISchema.h"

namespace graphql::mapi::object {
namespace methods::ConversationHas {

template <class TImpl>
concept getIdWithParams = requires (TImpl impl, service::FieldParams params)
{
	{ service::AwaitableScalar<response::IdType> { impl.getId(std::move(params)) } };
};

template <class TImpl>
concept getId = requires (TImpl impl)
{
	{ service::AwaitableScalar<response::IdType> { impl.getId() } };
};

template <class TImpl>
concept getSubjectWithParams = requires (TImpl impl, service::FieldParams params)
{
	{ service::AwaitableScalar<std::string> { impl.getSubject(std::move(params)) } };
};

template <class TImpl>
concept getSubject = requires (TImpl impl)
{
	{ service::AwaitableScalar<std::string> { impl.getSubject() } };
};

template <class TImpl>
concept getCountWithParams = requires (TImpl impl, service::FieldParams params)
{
	{ service::AwaitableScalar<int> { impl.getCount(std::move(params)) } };
};

template <class TImpl>
concept getCount = requires (TImpl impl)
{
	{ service::AwaitableScalar<int> { impl.getCount() } };
};

template <class TImpl>
concept getUnreadWithParams = requires (TImpl impl, service::FieldParams params)
{
	{ service::AwaitableScalar<int> { impl.getUnread(std::move(params)) } };
};

template <class TImpl>
concept getUnread = requires (TImpl impl)
{
	{ service::AwaitableScalar<int> { impl.getUnread() } };
};

template <class TImpl>
concept getReceivedWithParams = requires (TImpl impl, service::FieldParams params)
{
	{ service::AwaitableScalar<std::optional<response::Value>> { impl.getReceived(std::move(params)) } };
};

template <class TImpl>
concept getReceived = requires (TImpl impl)
{
	{ service::AwaitableScalar<std::optional<response::Value>> { impl.getReceived() } };
};

template <class TImpl>
concept getItemsWithParams = requires (TImpl impl, service::FieldParams params, std::optional<std::vector<response::IdType>> idsArg)
{
	{ service::AwaitableObject<std::vector<std::shared_ptr<Item>>> { impl.getItems(std::move(params), std::move(idsArg)) } };
};

template <class TImpl>
concept getItems = requires (TImpl impl, std::optional<std::vector<response::IdType>> idsArg)
{
	{ service::AwaitableObject<std::vector<std::shared_ptr<Item>>> { impl.getItems(std::move(idsArg)) } };
};

template <class TImpl>
concept beginSelectionSet = requires (TImpl impl, const service::SelectionSetParams params)
{
	{ impl.beginSelectionSet(params) };
};

template <class TImpl>
concept endSelectionSet = requires (TImpl impl, const service::SelectionSetParams params)
{
	{ impl.endSelectionSet(params) };
};

} // namespace methods::ConversationHas

class [[nodiscard("unnecessary construction")]] Conversation final
	: public service::Object
{
private:
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveId(service::ResolverParams&& params) const;
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveSubject(service::ResolverParams&& params) const;
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveCount(service::ResolverParams&& params) const;
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveUnread(service::ResolverParams&& params) const;
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveReceived(service::ResolverParams&& params) const;
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveItems(service::ResolverParams&& params) const;

	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolve_typename(service::ResolverParams&& params) const;

	struct [[nodiscard("unnecessary construction")]] Concept
	{
		virtual ~Concept() = default;

		virtual void beginSelectionSet(const service::SelectionSetParams& params) const = 0;
		virtual void endSelectionSet(const service::SelectionSetParams& params) const = 0;

		[[nodiscard("unnecessary call")]] virtual service::AwaitableScalar<response::IdType> getId(service::FieldParams&& params) const = 0;
		[[nodiscard("unnecessary call")]] virtual service::AwaitableScalar<std::string> getSubject(service::FieldParams&& params) const = 0;
		[[nodiscard("unnecessary call")]] virtual service::AwaitableScalar<int> getCount(service::FieldParams&& params) const = 0;
		[[nodiscard("unnecessary call")]] virtual service::AwaitableScalar<int> getUnread(service::FieldParams&& params) const = 0;
		[[nodiscard("unnecessary call")]] virtual service::AwaitableScalar<std::optional<response::Value>> getReceived(service::FieldParams&& params) const = 0;
		[[nodiscard("unnecessary call")]] virtual service::AwaitableObject<std::vector<std::shared_ptr<Item>>> getItems(service::FieldParams&& params, std::optional<std::vector<response::IdType>>&& idsArg) const = 0;
	};

	template <class T>
	struct [[nodiscard("unnecessary construction")]] Model final
		: Concept
	{
		explicit Model(std::shared_ptr<T> pimpl) noexcept
			: _pimpl { std::move(pimpl) }
		{
		}

		[[nodiscard("unnecessary call")]] service::AwaitableScalar<response::IdType> getId(service::FieldParams&& params) const override
		{
			if constexpr (methods::ConversationHas::getIdWithParams<T>)
			{
				return { _pimpl->getId(std::move(params)) };
			}
			else if constexpr (methods::ConversationHas::getId<T>)
			{
				return { _pimpl->getId() };
			}
			else
			{
				throw service::unimplemented_method(R"ex(Conversation::getId)ex");
			}
		}

		[[nodiscard("unnecessary call")]] service::AwaitableScalar<std::string> getSubject(service::FieldParams&& params) const override
		{
			if constexpr (methods::ConversationHas::getSubjectWithParams<T>)
			{
				return { _pimpl->getSubject(std::move(params)) };
			}
			else if constexpr (methods::ConversationHas::getSubject<T>)
			{
				return { _pimpl->getSubject() };
			}
			else
			{
				throw service::unimplemented_method(R"ex(Conversation::getSubject)ex");
			}
		}

		[[nodiscard("unnecessary call")]] service::AwaitableScalar<int> getCount(service::FieldParams&& params) const override
		{
			if constexpr (methods::ConversationHas::getCountWithParams<T>)
			{
				return { _pimpl->getCount(std::move(params)) };
			}
			else if constexpr (methods::ConversationHas::getCount<T>)
			{
				return { _pimpl->getCount() };
			}
			else
			{
				throw service::unimplemented_method(R"ex(Conversation::getCount)ex");
			}
		}

		[[nodiscard("unnecessary call")]] service::AwaitableScalar<int> getUnread(service::FieldParams&& params) const override
		{
			if constexpr (methods::ConversationHas::getUnreadWithParams<T>)
			{
				return { _pimpl->getUnread(std::move(params)) };
			}
			else if constexpr (methods::ConversationHas::getUnread<T>)
			{
				return { _pimpl->getUnread() };
			}
			else
			{
				throw service::unimplemented_method(R"ex(Conversation::getUnread)ex");
			}
		}

		[[nodiscard("unnecessary call")]] service::AwaitableScalar<std::optional<response::Value>> getReceived(service::FieldParams&& params) const override
		{
			if constexpr (methods::ConversationHas::getReceivedWithParams<T>)
			{
				return { _pimpl->getReceived(std::move(params)) };
			}
			else if constexpr (methods::ConversationHas::getReceived<T>)
			{
				return { _pimpl->getReceived() };
			}
			else
			{
				throw service::unimplemented_method(R"ex(Conversation::getReceived)ex");
			}
		}

		[[nodiscard("unnecessary call")]] service::AwaitableObject<std::vector<std::shared_ptr<Item>>> getItems(service::FieldParams&& params, std::optional<std::vector<response::IdType>>&& idsArg) const override
		{
			if constexpr (methods::ConversationHas::getItemsWithParams<T>)
			{
				return { _pimpl->getItems(std::move(params), std::move(idsArg)) };
			}
			else if constexpr (methods::ConversationHas::getItems<T>)
			{
				return { _pimpl->getItems(std::move(idsArg)) };
			}
			else
			{
				throw service::unimplemented_method(R"ex(Conversation::getItems)ex");
			}
		}

		void beginSelectionSet(const service::SelectionSetParams& params) const override
		{
			if constexpr (methods::ConversationHas::beginSelectionSet<T>)
			{
				_pimpl->beginSelectionSet(params);
			}
		}

		void endSelectionSet(const service::SelectionSetParams& params) const override
		{
			if constexpr (methods::ConversationHas::endSelectionSet<T>)
			{
				_pimpl->endSelectionSet(params);
			}
		}

	private:
		const std::shared_ptr<T> _pimpl;
	};

	explicit Conversation(std::unique_ptr<const Concept> pimpl) noexcept;

	[[nodiscard("unnecessary call")]] service::TypeNames getTypeNames() const noexcept;
	[[nodiscard("unnecessary call")]] service::ResolverMap getResolvers() const noexcept;

	void beginSelectionSet(const service::SelectionSetParams& params) const override;
	void endSelectionSet(const service::SelectionSetParams& params) const override;

	const std::unique_ptr<const Concept> _pimpl;

public:
	template <class T>
	explicit Conversation(std::shared_ptr<T> pimpl) noexcept
		: Conversation { std::unique_ptr<const Concept> { std::make_unique<Model<T>>(std::move(pimpl)) } }
	{
	}

	[[nodiscard("unnecessary call")]] static constexpr std::string_view getObjectType() noexcept
	{
		return { R"gql(Conversation)gql" };
	}
};

} // namespace graphql::mapi::object

#endif // CONVERSATIONOBJECT_H
