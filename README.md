# ns-allinone-3.43-NetAnim
The version ns-3.43 was released on October 9, 2024. Like all previous versions, it bundles all necessary libraries and dependencies for ns3. Pre-packaged, includes dependencies, easy to install, ideal for quickly testing tools like NetAnim and PyViz for visualizing network behavior in ns3. For further information please check [ns-3.43](https://www.nsnam.org/releases/ns-3-43/).


## Application to generate NetAnim trace files and run simulation
I have created two simple test cases to run simulation for 8 and 16 nodes. Both test cases will generate NetAnim trace files. Please follow the below given steps:
Following steps:

- Clone the repository:


```shell
git clone https://github.com/bk-TurbaAI/ns-allinone-3.43.git
cd ns-allinone-3.43
```


- Build:

```shell
./build.py --enable-examples --enable-tests
cd ns-3.43
```


- Run test script:

```shell
./test.py
```


- Generate the NetAnim trace files:


```shell
./ns-3
./ns3 run scratch/test-eight-nodes.cc
./ns3 run scratch/test-sixteen-nodes.cc
```

- Run the NetAnim tool:
```shell
cd ns-allinone-3.43/ns-allinone-3.43/netanim-3.109
./NetAnim
```

