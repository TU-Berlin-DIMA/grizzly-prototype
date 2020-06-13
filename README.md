# Grizzly: Efficient Stream Processing Through Adaptive Query Compilation

This repository provides a prototypical snapshot of the Grizzly code generator for stream processing.
This codebase contains a reduced functionality but illustrates our code generation approach. 
Currently, we integrate an advanced version of Grizzly in NebulaStream. Our new stream processing engine for the internet-of-things. To learn more about NebulaStream, please visit our https://www.nebula.stream.

- Paper: [Grizzly: Efficient Stream Processing Through Adaptive Query Compilation](https://www.nebula.stream/publications/grizzly.html)

- BibTeX citation:
```
@inproceedings{grulich2020grizzly,
author = {Grulich, Philipp M. and Sebastian, Bre\ss{} and Zeuch, Steffen and Traub, Jonas and Bleichert, Janis von and Chen, Zongxiong and Rabl, Tilmann and Markl, Volker},
title = {Grizzly: Efficient Stream Processing Through Adaptive Query Compilation},
year = {2020},
isbn = {9781450367356},
publisher = {Association for Computing Machinery},
address = {New York, NY, USA},
url = {https://doi.org/10.1145/3318464.3389739},
doi = {10.1145/3318464.3389739},
booktitle = {Proceedings of the 2020 ACM SIGMOD International Conference on Management of Data},
pages = {2487–2503},
numpages = {17},
location = {Portland, OR, USA},
series = {SIGMOD ’20}
}
```

## Features of this prototype:

- Adaptive code generation, with online data profiling.
- Filter, Map, Select, and Window Operators.
- Tumbling and sliding processing time windows.
- Sum, Count, Min, Max, Avg aggregation functions.



## How to build and run

### Dependencies
- Boost > 1.49
- Clang
- TBB

### Build the source code
1. Create a directory for your build system.
2. Call CMake to create a build system.
3. Use the generated Makefile to build the source code.
    ````
    # Debug Build
    mkdir debug-build && cd debug-build
    cmake -DCMAKE_BUILD_TYPE=Debug ..
    make -j
    
    # Release Build
    release-build && cd release-build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make -j
    ````

### Generate data
1. Customize the DataGenerator.cpp to the input schema of your query.
Default schema:
    ```C++
    Schema::create()
        /** Id of auction this bid is for. */
        .addFixSizeField("auction", DataType::Long, Stream)
        /** Id of person bidding in auction. */
        .addFixSizeField("bidder", DataType::Long, Stream)
        /** Price of bid, in cents. */
        .addFixSizeField("price", DataType::Long, Stream)
        /**Time at which bid was made (ms since epoch)*/
        .addFixSizeField("dateTime", DataType::Long, Stream);
    ```
2. Build the data generator with:
    ```sh
    cd data-generator && make 
    ```
3. Generate input data:
   ```
   ./dataGenerator $NumberOfTuple $NumberOfPersons $NumberOfAuctions
   ```

### Run a query
Queries are defined in the start.cpp. Thus, after chaining the query, you have to build the project again.
To start query execute the following command:
```shell
./grizzly $parallelism $numberOfInputTuple $experimentDuration $inputFilePath

For instance:
./grizzly 4 100000 60 ../data-generator/nexmark_test_data.bin
```

### Inspect generated code
Grizzly generates C++ code, which is stored under `$build_folder/jit-generated-code`. 
Furthermore, we can differentiate between three types of code artifacts. 
The last number in the file name indicates the execution stage.
```
0 = DEFAULT, 
1 = INSTRUMENTED,
2 = OPTIMIZED
```
