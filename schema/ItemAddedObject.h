// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#pragma once

#ifndef ITEMADDEDOBJECT_H
#define ITEMADDEDOBJECT_H

#include "MAPISchema.h"

namespace graphql::mapi::object {
namespace implements {

template <class I>
concept ItemAddedIs = std::is_same_v<I, ItemChange>;

} // namespace implements

namespace methods::ItemAddedHas {

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
concept getAddedWithParams = requires (TImpl impl, service::FieldParams params)
{
	{ service::AwaitableObject<std::shared_ptr<Item>> { impl.getAdded(std::move(params)) } };
};

template <class TImpl>
concept getAdded = requires (TImpl impl)
{
	{ service::AwaitableObject<std::shared_ptr<Item>> { impl.getAdded() } };
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

} // namespace methods::ItemAddedHas

class [[nodiscard("unnecessary construction")]] ItemAdded final
	: public service::Object
{
private:
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveIndex(service::ResolverParams&& params) const;
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveAdded(service::ResolverParams&& params) const;

	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolve_typename(service::ResolverParams&& params) const;

	struct [[nodiscard("unnecessary construction")]] Concept
	{
		virtual ~Concept() = default;

		virtual void beginSelectionSet(const service::SelectionSetParams& params) const = 0;
		virtual void endSelectionSet(const service::SelectionSetParams& params) const = 0;

		[[nodiscard("unnecessary call")]] virtual service::AwaitableScalar<int> getIndex(service::FieldParams&& params) const = 0;
		[[nodiscard("unnecessary call")]] virtual service::AwaitableObject<std::shared_ptr<Item>> getAdded(service::FieldParams&& params) const = 0;
	};

	template <class T>
	struct [[nodiscard("unnecessary construction")]] Model final
		: Concept
	{
		explicit Model(std::shared_ptr<T> pimpl) noexcept
			: _pimpl { std::move(pimpl) }
		{
		}

		[[nodiscard("unnecessary call")]] service::AwaitableScalar<int> getIndex(service::FieldParams&& params) const override
		{
			if constexpr (methods::ItemAddedHas::getIndexWithParams<T>)
			{
				return { _pimpl->getIndex(std::move(params)) };
			}
			else if constexpr (methods::ItemAddedHas::getIndex<T>)
			{
				return { _pimpl->getIndex() };
			}
			else
			{
				throw service::unimplemented_method(R"ex(ItemAdded::getIndex)ex");
			}
		}

		[[nodiscard("unnecessary call")]] service::AwaitableObject<std::shared_ptr<Item>> getAdded(service::FieldParams&& params) const override
		{
			if constexpr (methods::ItemAddedHas::getAddedWithParams<T>)
			{
				return { _pimpl->getAdded(std::move(params)) };
			}
			else if constexpr (methods::ItemAddedHas::getAdded<T>)
			{
				return { _pimpl->getAdded() };
			}
			else
			{
				throw service::unimplemented_method(R"ex(ItemAdded::getAdded)ex");
			}
		}

		void beginSelectionSet(const service::SelectionSetParams& params) const override
		{
			if constexpr (methods::ItemAddedHas::beginSelectionSet<T>)
			{
				_pimpl->beginSelectionSet(params);
			}
		}

		void endSelectionSet(const service::SelectionSetParams& params) const override
		{
			if constexpr (methods::ItemAddedHas::endSelectionSet<T>)
			{
				_pimpl->endSelectionSet(params);
			}
		}

	private:
		const std::shared_ptr<T> _pimpl;
	};

	explicit ItemAdded(std::unique_ptr<const Concept> pimpl) noexcept;

	// Unions which include this type
	friend ItemChange;

	template <class I>
	[[nodiscard("unnecessary call")]] static constexpr bool implements() noexcept
	{
		return implements::ItemAddedIs<I>;
	}

	[[nodiscard("unnecessary call")]] service::TypeNames getTypeNames() const noexcept;
	[[nodiscard("unnecessary call")]] service::ResolverMap getResolvers() const noexcept;

	void beginSelectionSet(const service::SelectionSetParams& params) const override;
	void endSelectionSet(const service::SelectionSetParams& params) const override;

	const std::unique_ptr<const Concept> _pimpl;

public:
	template <class T>
	explicit ItemAdded(std::shared_ptr<T> pimpl) noexcept
		: ItemAdded { std::unique_ptr<const Concept> { std::make_unique<Model<T>>(std::move(pimpl)) } }
	{
	}

	[[nodiscard("unnecessary call")]] static constexpr std::string_view getObjectType() noexcept
	{
		return { R"gql(ItemAdded)gql" };
	}
};

} // namespace graphql::mapi::object

#endif // ITEMADDEDOBJECT_H
