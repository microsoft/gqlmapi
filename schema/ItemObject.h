// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#pragma once

#ifndef ITEMOBJECT_H
#define ITEMOBJECT_H

#include "MAPISchema.h"

namespace graphql::mapi::object {
namespace implements {

template <class I>
concept ItemIs = std::is_same_v<I, Attachment>;

} // namespace implements

namespace methods::ItemHas {

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
concept getParentFolderWithParams = requires (TImpl impl, service::FieldParams params)
{
	{ service::AwaitableObject<std::shared_ptr<Folder>> { impl.getParentFolder(std::move(params)) } };
};

template <class TImpl>
concept getParentFolder = requires (TImpl impl)
{
	{ service::AwaitableObject<std::shared_ptr<Folder>> { impl.getParentFolder() } };
};

template <class TImpl>
concept getConversationWithParams = requires (TImpl impl, service::FieldParams params)
{
	{ service::AwaitableObject<std::shared_ptr<Conversation>> { impl.getConversation(std::move(params)) } };
};

template <class TImpl>
concept getConversation = requires (TImpl impl)
{
	{ service::AwaitableObject<std::shared_ptr<Conversation>> { impl.getConversation() } };
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
concept getSenderWithParams = requires (TImpl impl, service::FieldParams params)
{
	{ service::AwaitableScalar<std::optional<std::string>> { impl.getSender(std::move(params)) } };
};

template <class TImpl>
concept getSender = requires (TImpl impl)
{
	{ service::AwaitableScalar<std::optional<std::string>> { impl.getSender() } };
};

template <class TImpl>
concept getToWithParams = requires (TImpl impl, service::FieldParams params)
{
	{ service::AwaitableScalar<std::optional<std::string>> { impl.getTo(std::move(params)) } };
};

template <class TImpl>
concept getTo = requires (TImpl impl)
{
	{ service::AwaitableScalar<std::optional<std::string>> { impl.getTo() } };
};

template <class TImpl>
concept getCcWithParams = requires (TImpl impl, service::FieldParams params)
{
	{ service::AwaitableScalar<std::optional<std::string>> { impl.getCc(std::move(params)) } };
};

template <class TImpl>
concept getCc = requires (TImpl impl)
{
	{ service::AwaitableScalar<std::optional<std::string>> { impl.getCc() } };
};

template <class TImpl>
concept getBodyWithParams = requires (TImpl impl, service::FieldParams params)
{
	{ service::AwaitableScalar<std::optional<response::Value>> { impl.getBody(std::move(params)) } };
};

template <class TImpl>
concept getBody = requires (TImpl impl)
{
	{ service::AwaitableScalar<std::optional<response::Value>> { impl.getBody() } };
};

template <class TImpl>
concept getReadWithParams = requires (TImpl impl, service::FieldParams params)
{
	{ service::AwaitableScalar<bool> { impl.getRead(std::move(params)) } };
};

template <class TImpl>
concept getRead = requires (TImpl impl)
{
	{ service::AwaitableScalar<bool> { impl.getRead() } };
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
concept getModifiedWithParams = requires (TImpl impl, service::FieldParams params)
{
	{ service::AwaitableScalar<std::optional<response::Value>> { impl.getModified(std::move(params)) } };
};

template <class TImpl>
concept getModified = requires (TImpl impl)
{
	{ service::AwaitableScalar<std::optional<response::Value>> { impl.getModified() } };
};

template <class TImpl>
concept getPreviewWithParams = requires (TImpl impl, service::FieldParams params)
{
	{ service::AwaitableScalar<std::optional<std::string>> { impl.getPreview(std::move(params)) } };
};

template <class TImpl>
concept getPreview = requires (TImpl impl)
{
	{ service::AwaitableScalar<std::optional<std::string>> { impl.getPreview() } };
};

template <class TImpl>
concept getColumnsWithParams = requires (TImpl impl, service::FieldParams params)
{
	{ service::AwaitableObject<std::vector<std::shared_ptr<Property>>> { impl.getColumns(std::move(params)) } };
};

template <class TImpl>
concept getColumns = requires (TImpl impl)
{
	{ service::AwaitableObject<std::vector<std::shared_ptr<Property>>> { impl.getColumns() } };
};

template <class TImpl>
concept getAttachmentsWithParams = requires (TImpl impl, service::FieldParams params, std::optional<std::vector<response::IdType>> idsArg)
{
	{ service::AwaitableObject<std::vector<std::shared_ptr<Attachment>>> { impl.getAttachments(std::move(params), std::move(idsArg)) } };
};

template <class TImpl>
concept getAttachments = requires (TImpl impl, std::optional<std::vector<response::IdType>> idsArg)
{
	{ service::AwaitableObject<std::vector<std::shared_ptr<Attachment>>> { impl.getAttachments(std::move(idsArg)) } };
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

} // namespace methods::ItemHas

class [[nodiscard("unnecessary construction")]] Item final
	: public service::Object
{
private:
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveId(service::ResolverParams&& params) const;
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveParentFolder(service::ResolverParams&& params) const;
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveConversation(service::ResolverParams&& params) const;
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveSubject(service::ResolverParams&& params) const;
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveSender(service::ResolverParams&& params) const;
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveTo(service::ResolverParams&& params) const;
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveCc(service::ResolverParams&& params) const;
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveBody(service::ResolverParams&& params) const;
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveRead(service::ResolverParams&& params) const;
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveReceived(service::ResolverParams&& params) const;
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveModified(service::ResolverParams&& params) const;
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolvePreview(service::ResolverParams&& params) const;
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveColumns(service::ResolverParams&& params) const;
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveAttachments(service::ResolverParams&& params) const;

	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolve_typename(service::ResolverParams&& params) const;

	struct [[nodiscard("unnecessary construction")]] Concept
	{
		virtual ~Concept() = default;

		virtual void beginSelectionSet(const service::SelectionSetParams& params) const = 0;
		virtual void endSelectionSet(const service::SelectionSetParams& params) const = 0;

		[[nodiscard("unnecessary call")]] virtual service::AwaitableScalar<response::IdType> getId(service::FieldParams&& params) const = 0;
		[[nodiscard("unnecessary call")]] virtual service::AwaitableObject<std::shared_ptr<Folder>> getParentFolder(service::FieldParams&& params) const = 0;
		[[nodiscard("unnecessary call")]] virtual service::AwaitableObject<std::shared_ptr<Conversation>> getConversation(service::FieldParams&& params) const = 0;
		[[nodiscard("unnecessary call")]] virtual service::AwaitableScalar<std::string> getSubject(service::FieldParams&& params) const = 0;
		[[nodiscard("unnecessary call")]] virtual service::AwaitableScalar<std::optional<std::string>> getSender(service::FieldParams&& params) const = 0;
		[[nodiscard("unnecessary call")]] virtual service::AwaitableScalar<std::optional<std::string>> getTo(service::FieldParams&& params) const = 0;
		[[nodiscard("unnecessary call")]] virtual service::AwaitableScalar<std::optional<std::string>> getCc(service::FieldParams&& params) const = 0;
		[[nodiscard("unnecessary call")]] virtual service::AwaitableScalar<std::optional<response::Value>> getBody(service::FieldParams&& params) const = 0;
		[[nodiscard("unnecessary call")]] virtual service::AwaitableScalar<bool> getRead(service::FieldParams&& params) const = 0;
		[[nodiscard("unnecessary call")]] virtual service::AwaitableScalar<std::optional<response::Value>> getReceived(service::FieldParams&& params) const = 0;
		[[nodiscard("unnecessary call")]] virtual service::AwaitableScalar<std::optional<response::Value>> getModified(service::FieldParams&& params) const = 0;
		[[nodiscard("unnecessary call")]] virtual service::AwaitableScalar<std::optional<std::string>> getPreview(service::FieldParams&& params) const = 0;
		[[nodiscard("unnecessary call")]] virtual service::AwaitableObject<std::vector<std::shared_ptr<Property>>> getColumns(service::FieldParams&& params) const = 0;
		[[nodiscard("unnecessary call")]] virtual service::AwaitableObject<std::vector<std::shared_ptr<Attachment>>> getAttachments(service::FieldParams&& params, std::optional<std::vector<response::IdType>>&& idsArg) const = 0;
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
			if constexpr (methods::ItemHas::getIdWithParams<T>)
			{
				return { _pimpl->getId(std::move(params)) };
			}
			else if constexpr (methods::ItemHas::getId<T>)
			{
				return { _pimpl->getId() };
			}
			else
			{
				throw service::unimplemented_method(R"ex(Item::getId)ex");
			}
		}

		[[nodiscard("unnecessary call")]] service::AwaitableObject<std::shared_ptr<Folder>> getParentFolder(service::FieldParams&& params) const override
		{
			if constexpr (methods::ItemHas::getParentFolderWithParams<T>)
			{
				return { _pimpl->getParentFolder(std::move(params)) };
			}
			else if constexpr (methods::ItemHas::getParentFolder<T>)
			{
				return { _pimpl->getParentFolder() };
			}
			else
			{
				throw service::unimplemented_method(R"ex(Item::getParentFolder)ex");
			}
		}

		[[nodiscard("unnecessary call")]] service::AwaitableObject<std::shared_ptr<Conversation>> getConversation(service::FieldParams&& params) const override
		{
			if constexpr (methods::ItemHas::getConversationWithParams<T>)
			{
				return { _pimpl->getConversation(std::move(params)) };
			}
			else if constexpr (methods::ItemHas::getConversation<T>)
			{
				return { _pimpl->getConversation() };
			}
			else
			{
				throw service::unimplemented_method(R"ex(Item::getConversation)ex");
			}
		}

		[[nodiscard("unnecessary call")]] service::AwaitableScalar<std::string> getSubject(service::FieldParams&& params) const override
		{
			if constexpr (methods::ItemHas::getSubjectWithParams<T>)
			{
				return { _pimpl->getSubject(std::move(params)) };
			}
			else if constexpr (methods::ItemHas::getSubject<T>)
			{
				return { _pimpl->getSubject() };
			}
			else
			{
				throw service::unimplemented_method(R"ex(Item::getSubject)ex");
			}
		}

		[[nodiscard("unnecessary call")]] service::AwaitableScalar<std::optional<std::string>> getSender(service::FieldParams&& params) const override
		{
			if constexpr (methods::ItemHas::getSenderWithParams<T>)
			{
				return { _pimpl->getSender(std::move(params)) };
			}
			else if constexpr (methods::ItemHas::getSender<T>)
			{
				return { _pimpl->getSender() };
			}
			else
			{
				throw service::unimplemented_method(R"ex(Item::getSender)ex");
			}
		}

		[[nodiscard("unnecessary call")]] service::AwaitableScalar<std::optional<std::string>> getTo(service::FieldParams&& params) const override
		{
			if constexpr (methods::ItemHas::getToWithParams<T>)
			{
				return { _pimpl->getTo(std::move(params)) };
			}
			else if constexpr (methods::ItemHas::getTo<T>)
			{
				return { _pimpl->getTo() };
			}
			else
			{
				throw service::unimplemented_method(R"ex(Item::getTo)ex");
			}
		}

		[[nodiscard("unnecessary call")]] service::AwaitableScalar<std::optional<std::string>> getCc(service::FieldParams&& params) const override
		{
			if constexpr (methods::ItemHas::getCcWithParams<T>)
			{
				return { _pimpl->getCc(std::move(params)) };
			}
			else if constexpr (methods::ItemHas::getCc<T>)
			{
				return { _pimpl->getCc() };
			}
			else
			{
				throw service::unimplemented_method(R"ex(Item::getCc)ex");
			}
		}

		[[nodiscard("unnecessary call")]] service::AwaitableScalar<std::optional<response::Value>> getBody(service::FieldParams&& params) const override
		{
			if constexpr (methods::ItemHas::getBodyWithParams<T>)
			{
				return { _pimpl->getBody(std::move(params)) };
			}
			else if constexpr (methods::ItemHas::getBody<T>)
			{
				return { _pimpl->getBody() };
			}
			else
			{
				throw service::unimplemented_method(R"ex(Item::getBody)ex");
			}
		}

		[[nodiscard("unnecessary call")]] service::AwaitableScalar<bool> getRead(service::FieldParams&& params) const override
		{
			if constexpr (methods::ItemHas::getReadWithParams<T>)
			{
				return { _pimpl->getRead(std::move(params)) };
			}
			else if constexpr (methods::ItemHas::getRead<T>)
			{
				return { _pimpl->getRead() };
			}
			else
			{
				throw service::unimplemented_method(R"ex(Item::getRead)ex");
			}
		}

		[[nodiscard("unnecessary call")]] service::AwaitableScalar<std::optional<response::Value>> getReceived(service::FieldParams&& params) const override
		{
			if constexpr (methods::ItemHas::getReceivedWithParams<T>)
			{
				return { _pimpl->getReceived(std::move(params)) };
			}
			else if constexpr (methods::ItemHas::getReceived<T>)
			{
				return { _pimpl->getReceived() };
			}
			else
			{
				throw service::unimplemented_method(R"ex(Item::getReceived)ex");
			}
		}

		[[nodiscard("unnecessary call")]] service::AwaitableScalar<std::optional<response::Value>> getModified(service::FieldParams&& params) const override
		{
			if constexpr (methods::ItemHas::getModifiedWithParams<T>)
			{
				return { _pimpl->getModified(std::move(params)) };
			}
			else if constexpr (methods::ItemHas::getModified<T>)
			{
				return { _pimpl->getModified() };
			}
			else
			{
				throw service::unimplemented_method(R"ex(Item::getModified)ex");
			}
		}

		[[nodiscard("unnecessary call")]] service::AwaitableScalar<std::optional<std::string>> getPreview(service::FieldParams&& params) const override
		{
			if constexpr (methods::ItemHas::getPreviewWithParams<T>)
			{
				return { _pimpl->getPreview(std::move(params)) };
			}
			else if constexpr (methods::ItemHas::getPreview<T>)
			{
				return { _pimpl->getPreview() };
			}
			else
			{
				throw service::unimplemented_method(R"ex(Item::getPreview)ex");
			}
		}

		[[nodiscard("unnecessary call")]] service::AwaitableObject<std::vector<std::shared_ptr<Property>>> getColumns(service::FieldParams&& params) const override
		{
			if constexpr (methods::ItemHas::getColumnsWithParams<T>)
			{
				return { _pimpl->getColumns(std::move(params)) };
			}
			else if constexpr (methods::ItemHas::getColumns<T>)
			{
				return { _pimpl->getColumns() };
			}
			else
			{
				throw service::unimplemented_method(R"ex(Item::getColumns)ex");
			}
		}

		[[nodiscard("unnecessary call")]] service::AwaitableObject<std::vector<std::shared_ptr<Attachment>>> getAttachments(service::FieldParams&& params, std::optional<std::vector<response::IdType>>&& idsArg) const override
		{
			if constexpr (methods::ItemHas::getAttachmentsWithParams<T>)
			{
				return { _pimpl->getAttachments(std::move(params), std::move(idsArg)) };
			}
			else if constexpr (methods::ItemHas::getAttachments<T>)
			{
				return { _pimpl->getAttachments(std::move(idsArg)) };
			}
			else
			{
				throw service::unimplemented_method(R"ex(Item::getAttachments)ex");
			}
		}

		void beginSelectionSet(const service::SelectionSetParams& params) const override
		{
			if constexpr (methods::ItemHas::beginSelectionSet<T>)
			{
				_pimpl->beginSelectionSet(params);
			}
		}

		void endSelectionSet(const service::SelectionSetParams& params) const override
		{
			if constexpr (methods::ItemHas::endSelectionSet<T>)
			{
				_pimpl->endSelectionSet(params);
			}
		}

	private:
		const std::shared_ptr<T> _pimpl;
	};

	explicit Item(std::unique_ptr<const Concept> pimpl) noexcept;

	// Unions which include this type
	friend Attachment;

	template <class I>
	[[nodiscard("unnecessary call")]] static constexpr bool implements() noexcept
	{
		return implements::ItemIs<I>;
	}

	[[nodiscard("unnecessary call")]] service::TypeNames getTypeNames() const noexcept;
	[[nodiscard("unnecessary call")]] service::ResolverMap getResolvers() const noexcept;

	void beginSelectionSet(const service::SelectionSetParams& params) const override;
	void endSelectionSet(const service::SelectionSetParams& params) const override;

	const std::unique_ptr<const Concept> _pimpl;

public:
	template <class T>
	explicit Item(std::shared_ptr<T> pimpl) noexcept
		: Item { std::unique_ptr<const Concept> { std::make_unique<Model<T>>(std::move(pimpl)) } }
	{
	}

	[[nodiscard("unnecessary call")]] static constexpr std::string_view getObjectType() noexcept
	{
		return { R"gql(Item)gql" };
	}
};

} // namespace graphql::mapi::object

#endif // ITEMOBJECT_H
