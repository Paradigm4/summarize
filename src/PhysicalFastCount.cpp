/*
**
* BEGIN_COPYRIGHT
*
* PARADIGM4 INC.
* This file is part of the Paradigm4 Enterprise SciDB distribution kit
* and may only be used with a valid Paradigm4 contract and in accord
* with the terms and conditions specified by that contract.
*
* Copyright (C) 2010 - 2015 Paradigm4 Inc.
* All Rights Reserved.
*
* END_COPYRIGHT
*/


#include <limits>
#include <sstream>
#include <memory>
#include <string>
#include <vector>
#include <ctype.h>

#include <system/Exceptions.h>
#include <system/SystemCatalog.h>
#include <system/Sysinfo.h>

#include <query/TypeSystem.h>
#include <query/FunctionDescription.h>
#include <query/FunctionLibrary.h>
#include <query/Operator.h>
#include <query/TypeSystem.h>
#include <query/FunctionLibrary.h>
#include <query/Operator.h>
#include <array/DBArray.h>
#include <array/Tile.h>
#include <array/TileIteratorAdaptors.h>
#include <util/Platform.h>

#include <boost/algorithm/string.hpp>
#include <boost/unordered_map.hpp>

#include "FastCountSettings.h"

#ifdef CPP11
using std::make_shared;
#else
using boost::make_shared;
#endif

using std::make_shared;
using boost::algorithm::is_from_range;

namespace scidb
{

static log4cxx::LoggerPtr logger(log4cxx::Logger::getLogger("scidb.fast_count"));

using namespace scidb;

static void EXCEPTION_ASSERT(bool cond)
{
    if (! cond)
    {
        throw SYSTEM_EXCEPTION(SCIDB_SE_INTERNAL, SCIDB_LE_ILLEGAL_OPERATION) << "Internal inconsistency";
    }
}

class PhysicalFastCount : public PhysicalOperator
{
public:
    PhysicalFastCount(std::string const& logicalName,
                  std::string const& physicalName,
                  Parameters const& parameters,
                   ArrayDesc const& schema):
        PhysicalOperator(logicalName, physicalName, parameters, schema)
    {}

    virtual bool changesDistribution(std::vector<ArrayDesc> const&) const
    {
        return true;
    }

#ifdef CPP11
    virtual RedistributeContext getOutputDistribution(std::vector<RedistributeContext> const&, std::vector<ArrayDesc> const&) const
    {
        return RedistributeContext(psUndefined);
    }
#else
    virtual ArrayDistribution getOutputDistribution(std::vector<ArrayDistribution> const&, std::vector<ArrayDesc> const&) const
    {
        return ArrayDistribution(psUndefined);
    }
#endif

    std::shared_ptr< Array> execute(std::vector< std::shared_ptr< Array> >& inputArrays, std::shared_ptr<Query> query)
    {
    	FastCountSettings settings (_parameters, false, query);

    	shared_ptr<Array>& input = inputArrays[0];
    	shared_ptr< Array> outArray;

    	//shared_ptr<Array> output(new MemArray(inputArrays->getArrayDesc(), query));
    	//shared_ptr<ArrayIterator> outputArrayIterator(output->getIterator(0));

    	std::shared_ptr<ConstArrayIterator> inputIterator = input->getConstIterator(0);
    	size_t count = 0;

    	while(!inputIterator-> end())
    	{
    		//Coordinates const& pos = inputIterator->getPosition();
    		std::shared_ptr<ConstChunkIterator> inputChunkIterator = inputIterator->getChunk().getConstIterator();
    		//if(!inputChunkIterator->end()) //just 1 value in chunk
    		//{
    			ConstChunk const& chunk = inputChunkIterator->getChunk();
    			count+= chunk.count();

    			//++(*inputChunkIterator);
    		//}

    		++(*inputIterator);
    	}

    	LOG4CXX_DEBUG(logger, "FASTCOUNT - Finished fast counting the chunk values");

        shared_ptr<Array> outputArray(new MemArray(_schema, query));

        shared_ptr<ArrayIterator> outputArrayIter = outputArray->getIterator(0);
        Coordinates position(1,0);

        position[0] = query->getInstanceID();

        shared_ptr<ChunkIterator> outputChunkIter = outputArrayIter->newChunk(position).getIterator(query, ChunkIterator::SEQUENTIAL_WRITE);
        outputChunkIter->setPosition(position);

        Value value;
        value.setUint64(count);
        outputChunkIter->writeItem(value);
        outputChunkIter->flush();

    	return outputArray;

    }
};

REGISTER_PHYSICAL_OPERATOR_FACTORY(PhysicalFastCount, "fast_count", "PhysicalFastCount");


} // end namespace scidb
