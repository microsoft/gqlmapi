// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#pragma once

#ifndef DATETIMEVALUEOBJECT_H
#define DATETIMEVALUEOBJECT_H

#include "MAPISchema.h"

namespace graphql::mapi::object {
namespace implements {

template <class I>
concept DateTimeValueIs = std::is_same_v<I, PropValue>;

} // namespace implements

namespace methods::DateTimeValueHas {

template <class TImpl>
concept getValueWithParams = requires (TImpl impl, service::FieldParams params)
{
	{ service::AwaitableScalar<response::Value> { impl.getValue(std::move(params)) } };
};

template <class TImpl>
concept getValue = requires (TImpl impl)
{
	{ service::AwaitableScalar<response::Value> { impl.getValue() } };
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

} // namespace methods::DateTimeValueHas

class [[nodiscard("unnecessary construction")]] DateTimeValue final
	: public service::Object
{
private:
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveValue(service::ResolverParams&& params) const;

	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolve_typename(service::ResolverParams&& params) const;

	struct [[nodiscard("unnecessary construction")]] Concept
	{
		virtual ~Concept() = default;

		virtual void beginSelectionSet(const service::SelectionSetParams& params) const = 0;
		virtual void endSelectionSet(const service::SelectionSetParams& params) const = 0;

		[[nodiscard("unnecessary call")]] virtual service::AwaitableScalar<response::Value> getValue(service::FieldParams&& params) const = 0;
	};

	template <class T>
	struct [[nodiscard("unnecessary construction")]] Model final
		: Concept
	{
		explicit Model(std::shared_ptr<T> pimpl) noexcept
			: _pimpl { std::move(pimpl) }
		{
		}

		[[nodiscard("unnecessary call")]] service::AwaitableScalar<response::Value> getValue(service::FieldParams&& params) const override
		{
			if constexpr (methods::DateTimeValueHas::getValueWithParams<T>)
			{
				return { _pimpl->getValue(std::move(params)) };
			}
			else if constexpr (methods::DateTimeValueHas::getValue<T>)
			{
				return { _pimpl->getValue() };
			}
			else
			{
				throw service::unimplemented_method(R"ex(DateTimeValue::getValue)ex");
			}
		}

		void beginSelectionSet(const service::SelectionSetParams& params) const override
		{
			if constexpr (methods::DateTimeValueHas::beginSelectionSet<T>)
			{
				_pimpl->beginSelectionSet(params);
			}
		}

		void endSelectionSet(const service::SelectionSetParams& params) const override
		{
			if constexpr (methods::DateTimeValueHas::endSelectionSet<T>)
			{
				_pimpl->endSelectionSet(params);
			}
		}

	private:
		const std::shared_ptr<T> _pimpl;
	};

	explicit DateTimeValue(std::unique_ptr<const Concept> pimpl) noexcept;

	// Unions which include this type
	friend PropValue;

	template <class I>
	[[nodiscard("unnecessary call")]] static constexpr bool implements() noexcept
	{
		return implements::DateTimeValueIs<I>;
	}

	[[nodiscard("unnecessary call")]] service::TypeNames getTypeNames() const noexcept;
	[[nodiscard("unnecessary call")]] service::ResolverMap getResolvers() const noexcept;

	void beginSelectionSet(const service::SelectionSetParams& params) const override;
	void endSelectionSet(const service::SelectionSetParams& params) const override;

	const std::unique_ptr<const Concept> _pimpl;

public:
	template <class T>
	explicit DateTimeValue(std::shared_ptr<T> pimpl) noexcept
		: DateTimeValue { std::unique_ptr<const Concept> { std::make_unique<Model<T>>(std::move(pimpl)) } }
	{
	}

	[[nodiscard("unnecessary call")]] static constexpr std::string_view getObjectType() noexcept
	{
		return { R"gql(DateTimeValue)gql" };
	}
};

} // namespace graphql::mapi::object

#endif // DATETIMEVALUEOBJECT_H
