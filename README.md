# ATTENTION

![Attention](http://static.lightwave3d.com/sdk/2015/python/_images/deprecated.jpg)

As of [SciDB 18.1](http://forum.paradigm4.com/t/scidb-release-18-1/2124), `summarize` is now part of SciDB core [(see documentation here)](https://paradigm4.atlassian.net/wiki/spaces/scidb/pages/241500293/summarize). This repo is kept around for historical purposes, and potentially for external users to report issues. 

This repo supports `summarize` for up till SciDB 16.9. 

# Summarize
Compute quick chunk density, size and skew statistics over SciDB arrays.

# Example
Some spacing was added for quicker readability:
```
$ iquery -naq "store(apply(build(<val:double> [x=1:10000000,1000000,0], random()), val2, iif(x%2=0, 'abc','def'), val3, 0), temp)"

$ iquery -aq "summarize(temp)"
{inst,attid}   att,    count,     bytes, chunks, min_count, avg_count, max_count, min_bytes,   avg_bytes, max_bytes
{0,0}        'all', 10000000, 170002720,     40,   1000000,     1e+06,   1000000,        48, 4.25007e+06,   9000072
```

# Options
Summarize accepts string-style boolean flag options of `per_attribute` and `per_instance` which both default to `0` and can be combined:
```
$ iquery -aq "summarize(temp, 'per_instance=1')"
{inst,attid} att,count,bytes,chunks,min_count,avg_count,max_count,min_bytes,avg_bytes,max_bytes
{0,0} 'all',3000000,51000816,12,1000000,1e+06,1000000,48,4.25007e+06,9000072
{1,0} 'all',3000000,51000816,12,1000000,1e+06,1000000,48,4.25007e+06,9000072
{2,0} 'all',2000000,34000544,8,1000000,1e+06,1000000,48,4.25007e+06,9000072
{3,0} 'all',2000000,34000544,8,1000000,1e+06,1000000,48,4.25007e+06,9000072

$ iquery -aq "summarize(temp, 'per_attribute=1')"
{inst,attid} att,count,bytes,chunks,min_count,avg_count,max_count,min_bytes,avg_bytes,max_bytes
{0,0} 'val',10000000,80000720,10,1000000,1e+06,1000000,8000072,8.00007e+06,8000072
{0,1} 'val2',10000000,90000720,10,1000000,1e+06,1000000,9000072,9.00007e+06,9000072
{0,2} 'val3',10000000,800,10,1000000,1e+06,1000000,80,80,80
{0,3} 'EmptyTag',10000000,480,10,1000000,1e+06,1000000,48,48,48

$ iquery -aq "summarize(temp, 'per_attribute=1', 'per_instance=1')"
{inst,attid} att,count,bytes,chunks,min_count,avg_count,max_count,min_bytes,avg_bytes,max_bytes
{0,0} 'val',3000000,24000216,3,1000000,1e+06,1000000,8000072,8.00007e+06,8000072
{0,1} 'val2',3000000,27000216,3,1000000,1e+06,1000000,9000072,9.00007e+06,9000072
{0,2} 'val3',3000000,240,3,1000000,1e+06,1000000,80,80,80
{0,3} 'EmptyTag',3000000,144,3,1000000,1e+06,1000000,48,48,48
{1,0} 'val',3000000,24000216,3,1000000,1e+06,1000000,8000072,8.00007e+06,8000072
{1,1} 'val2',3000000,27000216,3,1000000,1e+06,1000000,9000072,9.00007e+06,9000072
{1,2} 'val3',3000000,240,3,1000000,1e+06,1000000,80,80,80
{1,3} 'EmptyTag',3000000,144,3,1000000,1e+06,1000000,48,48,48
{2,0} 'val',2000000,16000144,2,1000000,1e+06,1000000,8000072,8.00007e+06,8000072
{2,1} 'val2',2000000,18000144,2,1000000,1e+06,1000000,9000072,9.00007e+06,9000072
{2,2} 'val3',2000000,160,2,1000000,1e+06,1000000,80,80,80
{2,3} 'EmptyTag',2000000,96,2,1000000,1e+06,1000000,48,48,48
{3,0} 'val',2000000,16000144,2,1000000,1e+06,1000000,8000072,8.00007e+06,8000072
{3,1} 'val2',2000000,18000144,2,1000000,1e+06,1000000,9000072,9.00007e+06,9000072
{3,2} 'val3',2000000,160,2,1000000,1e+06,1000000,80,80,80
{3,3} 'EmptyTag',2000000,96,2,1000000,1e+06,1000000,48,48,48
```

# Returned Fields
Described as follows:
 * inst (dimension): the logical ID of the instance returning the data
 * attid (dimension): the attribute ID or 0 when returning totals across attributes
 * att: the string attribute name or 'all'
 * count: the total count of non-empty cells - equal across multiple attributes
 * bytes: the total number of used bytes 
 * chunks: the total number of chunks - added up across multiple attributes
 * min/avg/max_count: statistics about the number of non-empty cells in each chunk
 * min/avg/max_bytes: statistics about the sizes of chunks

# Comparison to Other Tools
Summarize is one of several ways to get chunk sizing. We'll list some advantages and disadvantages.
## Advantages:
 * Very quick: data is gathered quickly from chunk headers; chunks are not scanned off disk. For arrays exceeding tens of terabytes, `op_count` may take a while and `summarize` will perform much faster.
 * Does not require stored arrays: for example, `summarize(apply(temp, b, random()))` is possible. The user can thus avoid a store operation. However, an implicit materialization - full or partial, depending on the query - still happens as this is executed.
 * Easy to summarize the latest version of an array. `list('chunk map')` is good at describing how much total space an array takes up, but to find the size of the most recent version, the query is much more complicated.
 * Measures exactly the binary size of SciDB's internal RLE format.
 * Easy: a memorable keyword and quick access. Simpler than `aggregate(filter(list('chunk map'), uaid=...)...,)`

## Disadvantages:
 * Does not return compressed size or compression ratio for arrays with additional (`zlib`) compression enabled. You can find that data `list('datastores')` or `list('chunk map')`
 * Does not return other on-disk size overhead for stored arrays (tombstones, free lists, etc are not counted)
 
# Installation
The easiest way is to first set up dev_tools (https://github.com/paradigm4/dev_tools). Follow the instructions there.
