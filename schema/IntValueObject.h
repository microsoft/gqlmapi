// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#pragma once

#ifndef INTVALUEOBJECT_H
#define INTVALUEOBJECT_H

#include "MAPISchema.h"

namespace graphql::mapi::object {
namespace implements {

template <class I>
concept IntValueIs = std::is_same_v<I, PropValue>;

} // namespace implements

namespace methods::IntValueHas {

template <class TImpl>
concept getValueWithParams = requires (TImpl impl, service::FieldParams params) 
{
	{ service::AwaitableScalar<int> { impl.getValue(std::move(params)) } };
};

template <class TImpl>
concept getValue = requires (TImpl impl) 
{
	{ service::AwaitableScalar<int> { impl.getValue() } };
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

} // namespace methods::IntValueHas

class IntValue
	: public service::Object
{
private:
	service::AwaitableResolver resolveValue(service::ResolverParams&& params) const;

	service::AwaitableResolver resolve_typename(service::ResolverParams&& params) const;

	struct Concept
	{
		virtual ~Concept() = default;

		virtual void beginSelectionSet(const service::SelectionSetParams& params) const = 0;
		virtual void endSelectionSet(const service::SelectionSetParams& params) const = 0;

		virtual service::AwaitableScalar<int> getValue(service::FieldParams&& params) const = 0;
	};

	template <class T>
	struct Model
		: Concept
	{
		Model(std::shared_ptr<T>&& pimpl) noexcept
			: _pimpl { std::move(pimpl) }
		{
		}

		service::AwaitableScalar<int> getValue(service::FieldParams&& params) const final
		{
			if constexpr (methods::IntValueHas::getValueWithParams<T>)
			{
				return { _pimpl->getValue(std::move(params)) };
			}
			else if constexpr (methods::IntValueHas::getValue<T>)
			{
				return { _pimpl->getValue() };
			}
			else
			{
				throw std::runtime_error(R"ex(IntValue::getValue is not implemented)ex");
			}
		}

		void beginSelectionSet(const service::SelectionSetParams& params) const final
		{
			if constexpr (methods::IntValueHas::beginSelectionSet<T>)
			{
				_pimpl->beginSelectionSet(params);
			}
		}

		void endSelectionSet(const service::SelectionSetParams& params) const final
		{
			if constexpr (methods::IntValueHas::endSelectionSet<T>)
			{
				_pimpl->endSelectionSet(params);
			}
		}

	private:
		const std::shared_ptr<T> _pimpl;
	};

	IntValue(std::unique_ptr<Concept>&& pimpl) noexcept;

	// Unions which include this type
	friend PropValue;

	template <class I>
	static constexpr bool implements() noexcept
	{
		return implements::IntValueIs<I>;
	}

	service::TypeNames getTypeNames() const noexcept;
	service::ResolverMap getResolvers() const noexcept;

	void beginSelectionSet(const service::SelectionSetParams& params) const final;
	void endSelectionSet(const service::SelectionSetParams& params) const final;

	const std::unique_ptr<Concept> _pimpl;

public:
	template <class T>
	IntValue(std::shared_ptr<T> pimpl) noexcept
		: IntValue { std::unique_ptr<Concept> { std::make_unique<Model<T>>(std::move(pimpl)) } }
	{
	}
};

} // namespace graphql::mapi::object

#endif // INTVALUEOBJECT_H
