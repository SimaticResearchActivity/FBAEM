# FBAE
Framework for Broadcast Algorithms Evaluation

## Introduction

*FBAE* is a software framework, developped in C++, to evaluate performances (latency and throughput) of Total-Order Broadcast Algorithms when there are no failures. It has been tested on Linux and Windows. It should also compile and execute on MacOS.

Thanks to *FBAE*, it is possible to evaluate (and thus compare) the performances of different Total-Order broadcast algorithms using different communication protocols (currently, only [ENet](http://enet.bespin.org), TCP should be coming soon).

## Compilation of *FBAE*

Clone *FBAE* repository and apply *cmake* procedure of [this document](http://www-inf.telecom-sudparis.eu/COURS/JIN/sir/new_site/Supports/Documents/ToolsTeachingUnit/toolsTeachingUnit.html).

## Executing *FBAE*

### Writing JSON site file
First, write a JSON site file defining the sites which will run instances of *FBAE*, reproducing the following example:

```json
{
    "sites": [
        {
            "tuple_element0": "localhost",
            "tuple_element1": 4096
        },
        {
            "tuple_element0": "localhost",
            "tuple_element1": 4097
        },
        {
            "tuple_element0": "localhost",
            "tuple_element1": 4098
        }
    ]
}
```

This file example defines 3 sites, each based on localhost, the first supposed to listen for connections on port 4096, the second one on port 4097, and the third one on port 4098.

Note `Resources` directory of *FBAE* repository contains samples of site files.

### Launch *fbae* executable
Once your site file is ready, you can run `fbae` executable according to the following usage:

```txt
fbae -a|--algo <algo_number> -c|--comm <communicationLayer_identifier> -h|--help -n|--nbMsg <number> -r|--rank <rank_number> -s|--size <size_in_bytes> -S|--site <siteFile_name> -v|--verbose
Where:
  -a|--algo <algo_number>                    Broadcast Algorithm
                                                0 = Sequencer based
  -c|--comm <communicationLayer_identifier>  Communication layer to be used
                                                e = Enet (reliable)
  -h|--help                                  Show help message
  -n|--nbMsg <number>                        Number of messages to be sent
  -r|--rank <rank_number>                    Rank of process in site file (if 99, all algorithm participants are executed within threads in current process)
  -s|--size <size_in_bytes>                  Size of messages sent by a client (must be in interval [22,65515])
  -S|--site <siteFile_name>                  Name (including path) of the sites file to be used
  -v|--verbose                               [optional] Verbose display required
```

For instance, you can open 3 terminals anr run:

- `./fbae -a 0 -c e -n 20 -r 2 -s 32 -S ../../Resources/sites_3_local.json` on terminal 2 (In this example, we first launch `fbae` executable with rank 2, because we want to invoke Sequencer total-order broadcast algorithm. And the role of the sequencer process is given to the last site specified in json file).
- `./fbae -a 0 -c e -n 20 -r 1 -s 32 -S ../../Resources/sites_3_local.json` on terminal 1
- `./fbae -a 0 -c e -n 20 -r 0 -s 32 -S ../../Resources/sites_3_local.json` on terminal 0

After a while (depending on the number of messages to be sent you specified), `fbae` displays the statistics observed for this process, e.g.:

```txt
algo,commLayer,nbMsg,rank,sizeMsg,siteFile,nbReceivedMsg,nbSentMsg,ratio nbRcv/nbSent,Average (in ms),Min,Q(0,25),Q(0,5),Q(0,75),Q(0,99),Q(0,999),Q(0,9999),Max
SequencerENet,100,99,32,../../Resources/sites_3_local.json,100,200,0.500000,0.241571,0.130782,0.239602,0.244068,0.248755,0.417237,0.417237,0.417237,0.417237
```

Note that, for testing purpose, it is possible to launch a single instance of `fbae` which will execute all activities in different threads. To do so, give value `99` to the rank, e.g. `./fbae -a 0 -c e -n 20 -r 99 -s 32 -S ../../Resources/sites_3_local.json`

## Current status of *FBAE*

### Total-Order broadcast algorithms

#### Sequencer-Based algorithm

In this algorithm, one site (the last one specified in the sites file) is given the role of Sequencer. The other sites specified in the site file are gicen the role of Broadcasters.

When a Broadcaster wants to broadcast a message, it sends its message to the Sequencer which sends it back to all Broadcasters, so that they can deliver it.

### Communication protocol

#### ENet

[ENet](http://enet.bespin.org) is a reliable UDP networking library. It is also able to guarantee FIFO on communication channels. We use all of these properties in *FBAE*.

## Extending *FABE*

### Adding another Total-Order broadcast algorithm

If you want to add another Total-Order broadcast algorithm, inspire yourself from:

- `SequencerAlgoLayer.h` to declare the class implementing your algorithm,
- `SequencerAlgoLayer.cpp` to define this class,
- `SequencerAlgoLayerMsg.h` to define the identifier and the structure of each message used by your algorithm.

### Adding another communication protocol

If you want to add another Total-Order broadcast algorithm, inspire yourself from:

- `EnetCommLayer.h` to declare the class implementing your usage of the protocol,
- `EnetCommLayer.h` to define this class,
- `EnetCommPeer.h` to declare the classe representing a peer in your usage of the protocol,
- `EnetCommPeer.cpp` to define this class.





