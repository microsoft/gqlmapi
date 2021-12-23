// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#pragma once

#ifndef ITEMREMOVEDOBJECT_H
#define ITEMREMOVEDOBJECT_H

#include "MAPISchema.h"

namespace graphql::mapi::object {
namespace implements {

template <class I>
concept ItemRemovedIs = std::is_same_v<I, ItemChange>;

} // namespace implements

namespace methods::ItemRemovedHas {

template <class TImpl>
concept getIndexWithParams = requires (TImpl impl, service::FieldParams params) 
{
	{ service::AwaitableScalar<int> { impl.getIndex(std::move(params)) } };
};

template <class TImpl>
concept getIndex = requires (TImpl impl) 
{
	{ service::AwaitableScalar<int> { impl.getIndex() } };
};

template <class TImpl>
concept getRemovedWithParams = requires (TImpl impl, service::FieldParams params) 
{
	{ service::AwaitableScalar<response::IdType> { impl.getRemoved(std::move(params)) } };
};

template <class TImpl>
concept getRemoved = requires (TImpl impl) 
{
	{ service::AwaitableScalar<response::IdType> { impl.getRemoved() } };
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

} // namespace methods::ItemRemovedHas

class ItemRemoved
	: public service::Object
{
private:
	service::AwaitableResolver resolveIndex(service::ResolverParams&& params) const;
	service::AwaitableResolver resolveRemoved(service::ResolverParams&& params) const;

	service::AwaitableResolver resolve_typename(service::ResolverParams&& params) const;

	struct Concept
	{
		virtual ~Concept() = default;

		virtual void beginSelectionSet(const service::SelectionSetParams& params) const = 0;
		virtual void endSelectionSet(const service::SelectionSetParams& params) const = 0;

		virtual service::AwaitableScalar<int> getIndex(service::FieldParams&& params) const = 0;
		virtual service::AwaitableScalar<response::IdType> getRemoved(service::FieldParams&& params) const = 0;
	};

	template <class T>
	struct Model
		: Concept
	{
		Model(std::shared_ptr<T>&& pimpl) noexcept
			: _pimpl { std::move(pimpl) }
		{
		}

		service::AwaitableScalar<int> getIndex(service::FieldParams&& params) const final
		{
			if constexpr (methods::ItemRemovedHas::getIndexWithParams<T>)
			{
				return { _pimpl->getIndex(std::move(params)) };
			}
			else if constexpr (methods::ItemRemovedHas::getIndex<T>)
			{
				return { _pimpl->getIndex() };
			}
			else
			{
				throw std::runtime_error(R"ex(ItemRemoved::getIndex is not implemented)ex");
			}
		}

		service::AwaitableScalar<response::IdType> getRemoved(service::FieldParams&& params) const final
		{
			if constexpr (methods::ItemRemovedHas::getRemovedWithParams<T>)
			{
				return { _pimpl->getRemoved(std::move(params)) };
			}
			else if constexpr (methods::ItemRemovedHas::getRemoved<T>)
			{
				return { _pimpl->getRemoved() };
			}
			else
			{
				throw std::runtime_error(R"ex(ItemRemoved::getRemoved is not implemented)ex");
			}
		}

		void beginSelectionSet(const service::SelectionSetParams& params) const final
		{
			if constexpr (methods::ItemRemovedHas::beginSelectionSet<T>)
			{
				_pimpl->beginSelectionSet(params);
			}
		}

		void endSelectionSet(const service::SelectionSetParams& params) const final
		{
			if constexpr (methods::ItemRemovedHas::endSelectionSet<T>)
			{
				_pimpl->endSelectionSet(params);
			}
		}

	private:
		const std::shared_ptr<T> _pimpl;
	};

	ItemRemoved(std::unique_ptr<Concept>&& pimpl) noexcept;

	// Unions which include this type
	friend ItemChange;

	template <class I>
	static constexpr bool implements() noexcept
	{
		return implements::ItemRemovedIs<I>;
	}

	service::TypeNames getTypeNames() const noexcept;
	service::ResolverMap getResolvers() const noexcept;

	void beginSelectionSet(const service::SelectionSetParams& params) const final;
	void endSelectionSet(const service::SelectionSetParams& params) const final;

	const std::unique_ptr<Concept> _pimpl;

public:
	template <class T>
	ItemRemoved(std::shared_ptr<T> pimpl) noexcept
		: ItemRemoved { std::unique_ptr<Concept> { std::make_unique<Model<T>>(std::move(pimpl)) } }
	{
	}
};

} // namespace graphql::mapi::object

#endif // ITEMREMOVEDOBJECT_H
