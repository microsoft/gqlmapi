// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#pragma once

#ifndef STOREOBJECT_H
#define STOREOBJECT_H

#include "MAPISchema.h"

namespace graphql::mapi::object {
namespace methods::StoreHas {

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
concept getNameWithParams = requires (TImpl impl, service::FieldParams params)
{
	{ service::AwaitableScalar<std::string> { impl.getName(std::move(params)) } };
};

template <class TImpl>
concept getName = requires (TImpl impl)
{
	{ service::AwaitableScalar<std::string> { impl.getName() } };
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
concept getRootFoldersWithParams = requires (TImpl impl, service::FieldParams params, std::optional<std::vector<response::IdType>> idsArg)
{
	{ service::AwaitableObject<std::vector<std::shared_ptr<Folder>>> { impl.getRootFolders(std::move(params), std::move(idsArg)) } };
};

template <class TImpl>
concept getRootFolders = requires (TImpl impl, std::optional<std::vector<response::IdType>> idsArg)
{
	{ service::AwaitableObject<std::vector<std::shared_ptr<Folder>>> { impl.getRootFolders(std::move(idsArg)) } };
};

template <class TImpl>
concept getSpecialFoldersWithParams = requires (TImpl impl, service::FieldParams params, std::vector<SpecialFolder> idsArg)
{
	{ service::AwaitableObject<std::vector<std::shared_ptr<Folder>>> { impl.getSpecialFolders(std::move(params), std::move(idsArg)) } };
};

template <class TImpl>
concept getSpecialFolders = requires (TImpl impl, std::vector<SpecialFolder> idsArg)
{
	{ service::AwaitableObject<std::vector<std::shared_ptr<Folder>>> { impl.getSpecialFolders(std::move(idsArg)) } };
};

template <class TImpl>
concept getFolderPropertiesWithParams = requires (TImpl impl, service::FieldParams params, response::IdType folderIdArg, std::optional<std::vector<Column>> idsArg)
{
	{ service::AwaitableObject<std::vector<std::shared_ptr<Property>>> { impl.getFolderProperties(std::move(params), std::move(folderIdArg), std::move(idsArg)) } };
};

template <class TImpl>
concept getFolderProperties = requires (TImpl impl, response::IdType folderIdArg, std::optional<std::vector<Column>> idsArg)
{
	{ service::AwaitableObject<std::vector<std::shared_ptr<Property>>> { impl.getFolderProperties(std::move(folderIdArg), std::move(idsArg)) } };
};

template <class TImpl>
concept getItemPropertiesWithParams = requires (TImpl impl, service::FieldParams params, response::IdType itemIdArg, std::optional<std::vector<Column>> idsArg)
{
	{ service::AwaitableObject<std::vector<std::shared_ptr<Property>>> { impl.getItemProperties(std::move(params), std::move(itemIdArg), std::move(idsArg)) } };
};

template <class TImpl>
concept getItemProperties = requires (TImpl impl, response::IdType itemIdArg, std::optional<std::vector<Column>> idsArg)
{
	{ service::AwaitableObject<std::vector<std::shared_ptr<Property>>> { impl.getItemProperties(std::move(itemIdArg), std::move(idsArg)) } };
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

} // namespace methods::StoreHas

class [[nodiscard("unnecessary construction")]] Store final
	: public service::Object
{
private:
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveId(service::ResolverParams&& params) const;
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveName(service::ResolverParams&& params) const;
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveColumns(service::ResolverParams&& params) const;
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveRootFolders(service::ResolverParams&& params) const;
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveSpecialFolders(service::ResolverParams&& params) const;
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveFolderProperties(service::ResolverParams&& params) const;
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveItemProperties(service::ResolverParams&& params) const;

	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolve_typename(service::ResolverParams&& params) const;

	struct [[nodiscard("unnecessary construction")]] Concept
	{
		virtual ~Concept() = default;

		virtual void beginSelectionSet(const service::SelectionSetParams& params) const = 0;
		virtual void endSelectionSet(const service::SelectionSetParams& params) const = 0;

		[[nodiscard("unnecessary call")]] virtual service::AwaitableScalar<response::IdType> getId(service::FieldParams&& params) const = 0;
		[[nodiscard("unnecessary call")]] virtual service::AwaitableScalar<std::string> getName(service::FieldParams&& params) const = 0;
		[[nodiscard("unnecessary call")]] virtual service::AwaitableObject<std::vector<std::shared_ptr<Property>>> getColumns(service::FieldParams&& params) const = 0;
		[[nodiscard("unnecessary call")]] virtual service::AwaitableObject<std::vector<std::shared_ptr<Folder>>> getRootFolders(service::FieldParams&& params, std::optional<std::vector<response::IdType>>&& idsArg) const = 0;
		[[nodiscard("unnecessary call")]] virtual service::AwaitableObject<std::vector<std::shared_ptr<Folder>>> getSpecialFolders(service::FieldParams&& params, std::vector<SpecialFolder>&& idsArg) const = 0;
		[[nodiscard("unnecessary call")]] virtual service::AwaitableObject<std::vector<std::shared_ptr<Property>>> getFolderProperties(service::FieldParams&& params, response::IdType&& folderIdArg, std::optional<std::vector<Column>>&& idsArg) const = 0;
		[[nodiscard("unnecessary call")]] virtual service::AwaitableObject<std::vector<std::shared_ptr<Property>>> getItemProperties(service::FieldParams&& params, response::IdType&& itemIdArg, std::optional<std::vector<Column>>&& idsArg) const = 0;
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
			if constexpr (methods::StoreHas::getIdWithParams<T>)
			{
				return { _pimpl->getId(std::move(params)) };
			}
			else if constexpr (methods::StoreHas::getId<T>)
			{
				return { _pimpl->getId() };
			}
			else
			{
				throw service::unimplemented_method(R"ex(Store::getId)ex");
			}
		}

		[[nodiscard("unnecessary call")]] service::AwaitableScalar<std::string> getName(service::FieldParams&& params) const override
		{
			if constexpr (methods::StoreHas::getNameWithParams<T>)
			{
				return { _pimpl->getName(std::move(params)) };
			}
			else if constexpr (methods::StoreHas::getName<T>)
			{
				return { _pimpl->getName() };
			}
			else
			{
				throw service::unimplemented_method(R"ex(Store::getName)ex");
			}
		}

		[[nodiscard("unnecessary call")]] service::AwaitableObject<std::vector<std::shared_ptr<Property>>> getColumns(service::FieldParams&& params) const override
		{
			if constexpr (methods::StoreHas::getColumnsWithParams<T>)
			{
				return { _pimpl->getColumns(std::move(params)) };
			}
			else if constexpr (methods::StoreHas::getColumns<T>)
			{
				return { _pimpl->getColumns() };
			}
			else
			{
				throw service::unimplemented_method(R"ex(Store::getColumns)ex");
			}
		}

		[[nodiscard("unnecessary call")]] service::AwaitableObject<std::vector<std::shared_ptr<Folder>>> getRootFolders(service::FieldParams&& params, std::optional<std::vector<response::IdType>>&& idsArg) const override
		{
			if constexpr (methods::StoreHas::getRootFoldersWithParams<T>)
			{
				return { _pimpl->getRootFolders(std::move(params), std::move(idsArg)) };
			}
			else if constexpr (methods::StoreHas::getRootFolders<T>)
			{
				return { _pimpl->getRootFolders(std::move(idsArg)) };
			}
			else
			{
				throw service::unimplemented_method(R"ex(Store::getRootFolders)ex");
			}
		}

		[[nodiscard("unnecessary call")]] service::AwaitableObject<std::vector<std::shared_ptr<Folder>>> getSpecialFolders(service::FieldParams&& params, std::vector<SpecialFolder>&& idsArg) const override
		{
			if constexpr (methods::StoreHas::getSpecialFoldersWithParams<T>)
			{
				return { _pimpl->getSpecialFolders(std::move(params), std::move(idsArg)) };
			}
			else if constexpr (methods::StoreHas::getSpecialFolders<T>)
			{
				return { _pimpl->getSpecialFolders(std::move(idsArg)) };
			}
			else
			{
				throw service::unimplemented_method(R"ex(Store::getSpecialFolders)ex");
			}
		}

		[[nodiscard("unnecessary call")]] service::AwaitableObject<std::vector<std::shared_ptr<Property>>> getFolderProperties(service::FieldParams&& params, response::IdType&& folderIdArg, std::optional<std::vector<Column>>&& idsArg) const override
		{
			if constexpr (methods::StoreHas::getFolderPropertiesWithParams<T>)
			{
				return { _pimpl->getFolderProperties(std::move(params), std::move(folderIdArg), std::move(idsArg)) };
			}
			else if constexpr (methods::StoreHas::getFolderProperties<T>)
			{
				return { _pimpl->getFolderProperties(std::move(folderIdArg), std::move(idsArg)) };
			}
			else
			{
				throw service::unimplemented_method(R"ex(Store::getFolderProperties)ex");
			}
		}

		[[nodiscard("unnecessary call")]] service::AwaitableObject<std::vector<std::shared_ptr<Property>>> getItemProperties(service::FieldParams&& params, response::IdType&& itemIdArg, std::optional<std::vector<Column>>&& idsArg) const override
		{
			if constexpr (methods::StoreHas::getItemPropertiesWithParams<T>)
			{
				return { _pimpl->getItemProperties(std::move(params), std::move(itemIdArg), std::move(idsArg)) };
			}
			else if constexpr (methods::StoreHas::getItemProperties<T>)
			{
				return { _pimpl->getItemProperties(std::move(itemIdArg), std::move(idsArg)) };
			}
			else
			{
				throw service::unimplemented_method(R"ex(Store::getItemProperties)ex");
			}
		}

		void beginSelectionSet(const service::SelectionSetParams& params) const override
		{
			if constexpr (methods::StoreHas::beginSelectionSet<T>)
			{
				_pimpl->beginSelectionSet(params);
			}
		}

		void endSelectionSet(const service::SelectionSetParams& params) const override
		{
			if constexpr (methods::StoreHas::endSelectionSet<T>)
			{
				_pimpl->endSelectionSet(params);
			}
		}

	private:
		const std::shared_ptr<T> _pimpl;
	};

	explicit Store(std::unique_ptr<const Concept> pimpl) noexcept;

	[[nodiscard("unnecessary call")]] service::TypeNames getTypeNames() const noexcept;
	[[nodiscard("unnecessary call")]] service::ResolverMap getResolvers() const noexcept;

	void beginSelectionSet(const service::SelectionSetParams& params) const override;
	void endSelectionSet(const service::SelectionSetParams& params) const override;

	const std::unique_ptr<const Concept> _pimpl;

public:
	template <class T>
	explicit Store(std::shared_ptr<T> pimpl) noexcept
		: Store { std::unique_ptr<const Concept> { std::make_unique<Model<T>>(std::move(pimpl)) } }
	{
	}

	[[nodiscard("unnecessary call")]] static constexpr std::string_view getObjectType() noexcept
	{
		return { R"gql(Store)gql" };
	}
};

} // namespace graphql::mapi::object

#endif // STOREOBJECT_H
