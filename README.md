# GqlMAPI

This project contains a library built with [CppGraphQLGen](https://github.com/microsoft/cppgraphqlgen)
which binds a [GraphQL](https://graphql.org/) [schema](./schema/mapi.graphql) to
[MAPI](https://en.wikipedia.org/wiki/MAPI).

The main purpose of this project is to demonstrate building a non-trivial C++ GraphQL service with
CppGraphQLGen. It may also be useful in diagnostic and debugging tools for
[Microsoft Outlook](https://en.wikipedia.org/wiki/Microsoft_Outlook). This library is not
officially supported by Microsoft, but MAPI itself is still supported as a backwards compatible
[integration API](https://docs.microsoft.com/en-us/office/client-developer/outlook/mapi/outlook-mapi-reference)
for Microsoft Outlook.

## Getting Started

You will need a Windows computer with a working installation of Microsoft Outlook for Windows,
since that is how MAPI is installed. You can use any C++ compiler toolchain which fully supports
C++17 on Windows, I recommend [Visual Studio 2019](https://visualstudio.microsoft.com/vs/) or
[Visual Studio Code](https://code.visualstudio.com/?wt.mc_id=DX_841432) with the
[CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools) and
[C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) extensions from
Microsoft.

When cloning this repository, you should perform a recursive clone to get the
[MAPIStubLibrary](https://github.com/stephenegriffin/MAPIStubLibrary) sub-module. If you have
already cloned the repository without the sub-module, you can clone it separately with the
following Git commands:

```shell
git submodule init
git submodule update
```

The build system uses [CMake](https://cmake.org/). If you are using Visual Studio 2019, it comes
with a supported version of CMake (>= 3.17.1) pre-installed. You can also install CMake separately,
and then you can use it with another editor like Visual Studio Code or
[Vim](https://en.wikipedia.org/wiki/Vim_(text_editor)).

## Contributing

This project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.opensource.microsoft.com.

When you submit a pull request, a CLA bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., status check, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## Trademarks

This project may contain trademarks or logos for projects, products, or services. Authorized use of Microsoft 
trademarks or logos is subject to and must follow 
[Microsoft's Trademark & Brand Guidelines](https://www.microsoft.com/en-us/legal/intellectualproperty/trademarks/usage/general).
Use of Microsoft trademarks or logos in modified versions of this project must not cause confusion or imply Microsoft sponsorship.
Any use of third-party trademarks or logos are subject to those third-party's policies.
