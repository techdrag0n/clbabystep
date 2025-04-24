# BabyStepGiantStep


First I dont claim to have writen this from scratch. It started with https://github.com/brichard19/BitCrack which I have stripped down and removed unneeded code. 


A tool for brute-forcing Bitcoin private keys. 
The main purpose of this project is to contribute to the effort of solving the
 [Bitcoin puzzle transaction](https://blockchain.info/tx/08389f34c98c606322740c0be6a7125d9860bb8d5cb182c02f98461e5fa6cd15): A transaction with 32 addresses that become increasingly difficult to crack.

Im taking a different approch than other baby step giant step searches by using the Y point rather than the X point.   

### Using BabyStepGiantStep

#### Usage

`clBabystep.exe` for OpenCL devices. This runs  on GPUs to build the babystep part of the database.
`giantstep.exe` for x86 cpus builts the giantstep part. 
### Note: **clBabystep.exe is still EXPERIMENTAL**, as users have reported critial bugs when running.

**Note for Intel users:**
  I don't know if im affected by this as much of the old code was removed.
There is bug in Intel's OpenCL implementation which affects BitCrack. Details here: https://github.com/brichard19/BitCrack/issues/123


```
clBabystep.exe [OPTIONS] [TARGETS]

Where [TARGETS] are one or more Bitcoin address

Options:

-o, --out FILE
    Append private keys to FILE, one per line

-d, --device N
    Use device with ID equal to N

-b, --blocks BLOCKS
    The number of CUDA blocks

-t, --threads THREADS
    Threads per block

-p, --points NUMBER
    Each thread will process NUMBER keys at a time

--keyspace KEYSPACE
    Specify the range of keys to search, where KEYSPACE is in the format,

	START:END start at key START, end at key END
	START:+COUNT start at key START and end at key START + COUNT
    :END start at key 1 and end at key END
	:+COUNT start at key 1 and end at key 1 + COUNT

--list-devices
    List available devices

```


A few variables have been left or added along with comments for future use. Others will be removed during further developement.
#### Examples

The simplest usage, the keyspace will begin at 0, and the CUDA parameters will be chosen automatically
```
xxBitCrack.exe 1FshYsUh3mqgsG29XpZ23eLjWV8Ur3VwH
```

Multiple keys can be searched at once with minimal impact to performance. Provide the keys on the command line, or in a file with one address per line
```
xxBitCrack.exe 1FshYsUh3mqgsG29XpZ23eLjWV8Ur3VwH 15JhYXn6Mx3oF4Y7PcTAv2wVVAuCFFQNiP 19EEC52krRUK1RkUAEZmQdjTyHT7Gp1TYT
```

To start the search at a specific private key, use the `--keyspace` option:

```
xxBitCrack.exe --keyspace 766519C977831678F0000000000 1FshYsUh3mqgsG29XpZ23eLjWV8Ur3VwH
```

The `--keyspace` option can also be used to search a specific range:

```
xxBitCrack.exe --keyspace 80000000:ffffffff 1FshYsUh3mqgsG29XpZ23eLjWV8Ur3VwH
```

To periodically save progress, the `--continue` option can be used. This is useful for recovering
after an unexpected interruption:

```
xxBitCrack.exe --keyspace 80000000:ffffffff 1FshYsUh3mqgsG29XpZ23eLjWV8Ur3VwH
...
GeForce GT 640   224/1024MB | 1 target 10.33 MKey/s (1,244,659,712 total) [00:01:58]
^C
xxBitCrack.exe --keyspace 80000000:ffffffff 1FshYsUh3mqgsG29XpZ23eLjWV8Ur3VwH
...
GeForce GT 640   224/1024MB | 1 target 10.33 MKey/s (1,357,905,920 total) [00:02:12]
```


Use the `-b,` `-t` and `-p` options to specify the number of blocks, threads per block, and keys per thread.
```
xxBitCrack.exe -b 32 -t 256 -p 16 1FshYsUh3mqgsG29XpZ23eLjWV8Ur3VwH
```

// see also todo optimize memory/threads 

### Choosing the right parameters for your device

GPUs have many cores. Work for the cores is divided into blocks. Each block contains threads.

There are 3 parameters that affect performance: blocks, threads per block, and keys per thread.


`blocks:` Should be a multiple of the number of compute units on the device. The default is 32.

`threads:` The number of threads in a block. This must be a multiple of 32. The default is 256.

`Keys per thread:` The number of keys each thread will process. The performance (keys per second)
increases asymptotically with this value. The default is 256. Increasing this value will cause the
kernel to run longer, but more keys will be processed.


### Build dependencies

Visual Studio 2022 (if on Windows)

For OpenCL: An OpenCL SDK (The CUDA toolkit contains an OpenCL SDK).


### Building in Windows

Open the Visual Studio solution.

Build the `clBabystep` project for an OpenCL build.

Note: By default the NVIDIA OpenCL headers are used. You can set the header and library path for
OpenCL in the `clBabystep.props` property sheet.




### Supporting this project  

If you find this project useful and would like to support it, consider making a donation. Your support is greatly appreciated!

**BTC**: `1Kej6yjTWjRNcqY2gzCY9iwwUcUTAbapTr`

**LTC**: `LQRVc5ESVK2tkdY2WPXD6zEWQ5BfZJTW5N`

**ETH**: `0xbf264fb0f20a3d2345856abcc27b7e2f40fca209`

### Contact

Send any questions or comments to babystepgiantstep@tek-dragon.com // change to my email
