// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#pragma once

#ifndef NAMEDPROPIDOBJECT_H
#define NAMEDPROPIDOBJECT_H

#include "MAPISchema.h"

namespace graphql::mapi::object {

class [[nodiscard("unnecessary construction")]] NamedPropId final
	: public service::Object
{
private:
	struct [[nodiscard("unnecessary construction")]] Concept
	{
		virtual ~Concept() = default;

		[[nodiscard("unnecessary call")]] virtual service::TypeNames getTypeNames() const noexcept = 0;
		[[nodiscard("unnecessary call")]] virtual service::ResolverMap getResolvers() const noexcept = 0;

		virtual void beginSelectionSet(const service::SelectionSetParams& params) const = 0;
		virtual void endSelectionSet(const service::SelectionSetParams& params) const = 0;
	};

	template <class T>
	struct [[nodiscard("unnecessary construction")]] Model final
		: Concept
	{
		explicit Model(std::shared_ptr<T> pimpl) noexcept
			: _pimpl { std::move(pimpl) }
		{
		}

		[[nodiscard("unnecessary call")]] service::TypeNames getTypeNames() const noexcept override
		{
			return _pimpl->getTypeNames();
		}

		[[nodiscard("unnecessary call")]] service::ResolverMap getResolvers() const noexcept override
		{
			return _pimpl->getResolvers();
		}

		void beginSelectionSet(const service::SelectionSetParams& params) const override
		{
			_pimpl->beginSelectionSet(params);
		}

		void endSelectionSet(const service::SelectionSetParams& params) const override
		{
			_pimpl->endSelectionSet(params);
		}

	private:
		const std::shared_ptr<T> _pimpl;
	};

	explicit NamedPropId(std::unique_ptr<const Concept> pimpl) noexcept;

	void beginSelectionSet(const service::SelectionSetParams& params) const override;
	void endSelectionSet(const service::SelectionSetParams& params) const override;

	const std::unique_ptr<const Concept> _pimpl;

public:
	template <class T>
	explicit NamedPropId(std::shared_ptr<T> pimpl) noexcept
		: NamedPropId { std::unique_ptr<const Concept> { std::make_unique<Model<T>>(std::move(pimpl)) } }
	{
		static_assert(T::template implements<NamedPropId>(), "NamedPropId is not implemented");
	}
};

} // namespace graphql::mapi::object

#endif // NAMEDPROPIDOBJECT_H
