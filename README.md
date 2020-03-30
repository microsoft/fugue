# Fugue
Fugue is a decoupled transaction component providing transaction processing for applications, primarily decoupled data stores and query runtime. It is elastic from multi-cores to multi-machines, and therefore can be used in on-premise environments as a library or in cloud. It aims to integrate with first- or third-party data stores and query runtime and make transaction processing pay-as-you-go for existing data management systems and platforms. The project explores a new business model that data management systems and platforms are assembled from separate, first- or third-party components/services (i.e., data stores, transaction and query runtime), and that we deliver one service at a time augmenting existing products or all assembled as a whole.

# Build

## Linux environment

#### Requirement

- [cmake](https://cmake.org/)
- If you want to run test code on cassandra data store: [cassandra](http://cassandra.apache.org/) 

#### Build

```bash
./bootstrap.sh

mkdir build && cd build
# optionally, use: -D CMAKE_BUILD_TYPE=Release or: -D CMAKE_BUILD_TYPE=Debug
cmake .. 
make
```

# Contributing

This project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.opensource.microsoft.com.

When you submit a pull request, a CLA bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., status check, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
