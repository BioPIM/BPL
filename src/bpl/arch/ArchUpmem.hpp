////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#ifndef __BPL_ARCH_UPMEM__
#define __BPL_ARCH_UPMEM__
////////////////////////////////////////////////////////////////////////////////

#include <string>
#include <array>
#include <map>
#include <sstream>
#include <memory>
#include <numeric>
#include <bpl/arch/Arch.hpp>
#include <bpl/arch/ArchUpmemResources.hpp>
#include <bpl/arch/ArchMulticore.hpp>
#include <bpl/arch/ArchMulticoreResources.hpp>
#include <bpl/utils/std.hpp>
#include <bpl/utils/serialize.hpp>
#include <bpl/utils/getname.hpp>
#include <bpl/utils/BufferIterator.hpp>
#include <bpl/utils/TaskUnit.hpp>
#include <bpl/utils/FileUtils.hpp>
#include <bpl/utils/TimeUtils.hpp>
#include <bpl/utils/splitter.hpp>
#include <bpl/utils/Statistics.hpp>
#include <config.hpp>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
namespace arch {
////////////////////////////////////////////////////////////////////////////////

/** Enumeration that distinguish sets made of ranks or and sets made of DPUs.
 */
enum DpuComponentKind_e
{
    /** Architecture made of ranks. */
    RANK,
    /** Architecture made of DPUs. */
    DPU,
    /** Architecture made of tasklets. */
    TASKLET
};

///////////////////////////////////////////////////////////////////////////////
// We hide some implementation details in a .pri file
#include <bpl/arch/ArchUpmem.pri>

///////////////////////////////////////////////////////////////////////////////
/** \brief Class that provides resources for the UPMEM architecture
 *
 * \see bpl::core::Launcher
 */
class ArchUpmem : public ArchMulticoreResources
{
public:

    /** To be improved (not really nice to have a 'kind' method that provide some typing info)
     */
    class Rank : public core::TaskUnit
    {
    public:
        using arch_t = ArchUpmem;
        using core::TaskUnit::TaskUnit;
        DpuComponentKind_e kind() const { return DpuComponentKind_e::RANK; }
        std::size_t getNbUnits() const { return getNbComponents()*64*NR_TASKLETS; }
        constexpr static int LEVEL = 1;
    };

    class DPU : public core::TaskUnit
    {
    public:
        using arch_t = ArchUpmem;
        using core::TaskUnit::TaskUnit;
        DpuComponentKind_e kind() const { return DpuComponentKind_e::DPU; }
        std::size_t getNbUnits() const { return getNbComponents()*NR_TASKLETS; }
        constexpr static int LEVEL = 2;
    };

    class Tasklet : public core::TaskUnit
    {
    public:
        using arch_t = ArchUpmem;
        using core::TaskUnit::TaskUnit;
        DpuComponentKind_e kind() const { return DpuComponentKind_e::TASKLET; }
        std::size_t getNbUnits() const { return getNbComponents(); }
        constexpr static int LEVEL = 3;
    };


    // Shortcut
    using arch_t = ArchUpmem;
    //using arch_t = ArchMulticore;

    using lowest_level_t = Tasklet;

    using Serializer = core::Serialize<arch_t,core::BufferIterator<ArchMulticore>,8>;

    /** Defines the granularity of the UPMEM components used by this logical view of the architecture.
     * This enumeration should be used only at construction.
     *
     * Note: we prefer here to manage an enumeration that acts like a type, which is in general no well-designed class;
     * we could have sub-classed ArchUpmem twice, with ArchUpmemRank and ArchUpmemDPU.
     * However, from the end user's point of view, it makes sense to have only one class name (i.e. ArchUpmem) and decide
     * at construction which kind of component we want to used. Moreover, it becomes possible to imagine configuring
     * an ArchUpmem object through a configuration file, which would be not possible with a direct call in the code to
     * either 'ArchUpmemRank' or 'ArchUpmemDPU'.
     */

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    /** Constructor.
     */
    template<typename TASKUNIT=Rank>
    ArchUpmem (
        TASKUNIT taskunit = TASKUNIT(1),
        bool trace=false,
        bool reset=false
    )
    {
        auto ts = statistics_.produceTimestamp("init/alloc");

        dpuSet_ = std::make_shared<details::dpu_set_handle_t> (taskunit.kind(),taskunit.getNbComponents(), nullptr, trace);

        // We may need to reset resources between each call to 'run' (like MRAM memory management for instance).
        reset_ = reset;

        statistics_.addTag ("dpu/options", dpuSet_->getOptions());
    }

    /** Return the name of the current architecture.
     * \return the architecture name
     */
    std::string name() { return "upmem"; }

    /** Return the total number of tasklets.
     * \return the number of process units
     */
    size_t getProcUnitNumber() const
    {
        return dpuSet_->getProcUnitNumber();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    /** Execute a task for different configurations.
     * \param[in] configurations: vector of tuples, each one holding the input parameters for the task
     */
    template<template<typename ...> class TASK, typename...TRAITS, typename ...ARGS>
    auto run (ARGS&&...args)
    {
        using task_t = TASK<arch_t,TRAITS...>;

        // Being here means that the input parameters should have already been broadcasted to the DPUs.

        auto ts = statistics_.produceTimestamp("run/all");

        DEBUG_ARCH_UPMEM ("[ArchUpmem::run]  BEGIN\n");

        // We determine the result type of the TASK::run method.
        // Note that the TASK type is instantiated with the current ARCH type.
        using result_t = bpl::core::return_t<decltype(&task_t::operator())>;

        // We call the 'prepare' method that will prepare the serialization
        prepare<TASK,TRAITS...> (std::forward<ARGS>(args)...);

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    auto ts_launch = statistics_.produceTimestamp("run/launch");
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        // We launch the execution.
        DPU_ASSERT(dpu_launch(set(), DPU_SYNCHRONOUS));

        statistics_.increment ("dpu_launch");

    //----------------------------------------------------------------------
    ts_launch.stop();
    //----------------------------------------------------------------------

        DEBUG_ARCH_UPMEM ("[ArchUpmem::run]  dpu_launch done\n");

        // We retrieve the results, first the size then the data.
        struct dpu_set_t dpu;
        uint32_t each_dpu;

        std::vector<uint32_t> allTaskletsOrder;
        std::vector<uint32_t> allTaskletsSize;
        std::vector<uint32_t> allDPUSize;
        core::AllocatorStats __allocator_stats__;

        std::vector<core::TimeStats> allNbCycles;
        uint32_t clocks_per_sec = 0;
        uint64_t totalResultSize = 0;
        uint64_t maxResultSize   = 0;

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    auto ts_get_output_info = statistics_.produceTimestamp("run/output_info_get");
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        DPU_FOREACH(set(), dpu, each_dpu)
        {
            std::array<uint32_t,NR_TASKLETS> taskletOrder;
            DPU_ASSERT(dpu_copy_from(dpu, "__result_tasklet_order__", 0, taskletOrder.data(), sizeof(taskletOrder)));
            for (auto idx : taskletOrder)
            {
                VERBOSE_ARCH_UPMEM ("[ArchUpmem::run]  __result_tasklet_order__:  idx=%d\n", idx);
                allTaskletsOrder.push_back (idx);
            }

            uint32_t DPUSize = 0;
            std::array<uint32_t,NR_TASKLETS> resultsSize;
            DPU_ASSERT(dpu_copy_from(dpu, "__result_tasklet_size__", 0, resultsSize.data(), sizeof(resultsSize)));
            for (auto s : resultsSize)
            {
                VERBOSE_ARCH_UPMEM ("[ArchUpmem::run]  __result_tasklet_size__ :  s=%d\n", s);
                allTaskletsSize.push_back (s);
                DPUSize += s;
            }
            allDPUSize.push_back (DPUSize);
            totalResultSize += DPUSize;

            if (DPUSize > maxResultSize)  { maxResultSize = DPUSize; }

            std::array<core::TimeStats,NR_TASKLETS> nbCycles;
            DPU_ASSERT(dpu_copy_from(dpu, "__nb_cycles__", 0, nbCycles.data(), sizeof(nbCycles)));
            for (const auto& val : nbCycles)
            {
                VERBOSE_ARCH_UPMEM ("[ArchUpmem::run]  __nb_cycles__:  [%d,%d,%d,%d,%d] \n",
                    val.unserialize,  val.split,  val.exec,  val.result,  val.all
                );
                allNbCycles.push_back (val);
            }

            DPU_ASSERT(dpu_copy_from(dpu, "CLOCKS_PER_SEC", 0, &clocks_per_sec,  sizeof(uint32_t)));

            DPU_ASSERT(dpu_copy_from(dpu, "__allocator_stats__", 0, &__allocator_stats__,  sizeof(__allocator_stats__)));
        }

        computeCyclesStats (allNbCycles, clocks_per_sec);

        statistics_.addTag ("resources/MRAM/used",        std::to_string(__allocator_stats__.used));
        statistics_.addTag ("resources/MRAM/calls/get",   std::to_string(__allocator_stats__.nbCallsGet));
        statistics_.addTag ("resources/MRAM/calls/read",  std::to_string(__allocator_stats__.nbCallsRead));

        statistics_.addTag ("resources/nbpu",      std::to_string(getProcUnitNumber()));

        // Possible check -> allTaskletsOrder should be equal to the sum from 0 to N-1 = N*(N-1)/2  (with N=getProcUnitNumber())
        DEBUG_ARCH_UPMEM ("[ArchUpmem::run]  totalResultSize=%ld  maxResultSize=%ld   #allTaskletsOrder=%ld  checksum=(%ld,%d) \n",
            totalResultSize, maxResultSize,
            allTaskletsOrder.size(),
            getProcUnitNumber()*(getProcUnitNumber()-1)/2,
            std::accumulate(allTaskletsOrder.begin(), allTaskletsOrder.end(), 0)
        );

    //----------------------------------------------------------------------
    ts_get_output_info.stop();
    //----------------------------------------------------------------------

        // We first retrieve heap pointer information.
        vector<uint32_t> heap_pointers (getProcUnitNumber());
        DPU_FOREACH(set(), dpu, each_dpu)
        {
            DPU_ASSERT(dpu_prepare_xfer(dpu, ((char*) heap_pointers.data()) + each_dpu*sizeof(uint32_t)));
        }
        DPU_ASSERT(dpu_push_xfer(set(), DPU_XFER_FROM_DPU, "__heap_pointer__", 0, sizeof(uint32_t), DPU_XFER_DEFAULT));

        struct Data
        {
            const ArchUpmem ref;
            const std::vector<uint32_t> heap_pointers;
            const std::vector<uint32_t> allDPUSize;
            const std::vector<uint32_t> allTaskletsSize;
            const std::vector<uint32_t> allTaskletsOrder;
        };

        struct iterable_wrapper
        {

            struct iterator
            {
                iterator (Data data, size_t idx, size_t last) : data_(data), idx_(idx), last_(last)
                {
                    // We initialize the DPU iterator.
                    dpu_it_ = dpu_set_dpu_iterator_from (& data.ref.set());

                    retrieve ();
                }

                bool operator!= (const size_t & sentinel) const { return idx_ != sentinel; }

                iterator& operator++ () { ++idx_;  retrieve();  return *this; }

                const result_t& operator* () const { return result_; }

                void retrieve ()
                {
                    if (idx_ < last_)
                    {
                        // Do we begin to retrieve information from a new DPU ?
                        if ((idx_%NR_TASKLETS) == 0 and dpu_it_.has_next)
                        {
                            size_t each_dpu = idx_ / NR_TASKLETS;

                            size_t bufferSize = data_.allDPUSize [each_dpu];

                            buffer_.resize (bufferSize);

                            offset_ = 0;

                            DPU_ASSERT(dpu_copy_from_mram (
                                dpu_it_.next.dpu,
                                (uint8_t*) buffer_.data(),
                                data_.heap_pointers [each_dpu],
                                bufferSize
                            ));

                            dpu_set_dpu_iterator_next (&dpu_it_);
                        }

                        size_t actualIdx = data_.allTaskletsOrder[idx_];

                        // We deserialize the current result from the incoming data
                        Serializer::from ((char*) buffer_.data() + offset_, result_);

                        offset_ += data_.allTaskletsSize[actualIdx];
                    }
                }

                Data data_;
                size_t   idx_;
                size_t   last_;
                result_t result_;
                std::vector<uint8_t> buffer_;
                size_t  offset_=0;

                struct dpu_set_dpu_iterator_t dpu_it_;
            };

            iterable_wrapper (Data data) : data_(data) {}

            auto begin() const  { return iterator (data_, 0, data_.ref.getProcUnitNumber()); }
            auto end()   const  { return size_t { data_.ref.getProcUnitNumber() }; }  // sentinel

            size_t size() const { return data_.ref.getProcUnitNumber(); }

            Data data_;
        };

        DEBUG_ARCH_UPMEM ("[ArchUpmem::run]  'results' filled\n");

        // we may want to dump the logs
        statistics_.dump();

        dpuSet_->dump();


        // We return the result.
        return iterable_wrapper (Data {*this,heap_pointers,allDPUSize,allTaskletsSize,allTaskletsOrder});
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    /** Prepare the input arguments into several configurations.
     * \param[in] args: input parameters for the task to be executed.
     * \return vector of tuples, each one holding an exec configuration
     */
    template<template<typename ...> class TASK, typename...TRAITS, typename ...ARGS>
    auto prepare (ARGS&&... args)
    {
        volatile auto ts_all = statistics_.produceTimestamp("prepare/all");

        using task_t = TASK<arch_t,TRAITS...>;

        using TARGS  = std::tuple<ARGS...>;

        DEBUG_ARCH_UPMEM ("[ArchUpmem::prepare]  BEGIN\n");

        // For each type in the 'run' prototype, retrieve the fact it is a reference or not
        //using fct_t  = decltype(bpl::core::GetParamType(TASK<arch_t>::run));
        //using fct_t  = decltype(bpl::core::GetParamType( TASK<arch_t>::operator() ) );

        using proto_t = std::decay_t<decltype(&task_t::operator())>;
        using allargs_t = typename bpl::core::FunctionSignatureParser<proto_t>::args_tuple;

        using args_t = allargs_t; // typename bpl::core::remove_first_type <allargs_t>::type;

        // 'firstTagOnceRefIdx' is only defined to check that potential 'once' args are the first ones.
        [[maybe_unused]] constexpr int firstTagOnceRefIdx   = bpl::core::first_predicate_idx_v <args_t, bpl::core::hastag_once,   true>;
        [[maybe_unused]] constexpr int firstNoTagOnceRefIdx = bpl::core::first_predicate_idx_v <args_t, bpl::core::hasnotag_once, false>;
        [[maybe_unused]] constexpr int hasTagOnce           = firstTagOnceRefIdx < std::tuple_size_v<args_t>;

        DEBUG_ARCH_UPMEM ("[ArchUpmem::prepare]  firstTagOnceRefIdx=%d   firstNoTagOnceRefIdx=%d  hasTagOnce=%d \n",
            firstTagOnceRefIdx, firstNoTagOnceRefIdx, hasTagOnce
        );

        // We serialize the tuple holding the input parameters.
        TARGS targs {std::forward<ARGS>(args)...};

        DEBUG_ARCH_UPMEM ("[ArchUpmem::prepare]  tuple init done\n");

        [[maybe_unused]] bool alreadyLoaded = false;

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    auto ts_serialize1 = statistics_.produceTimestamp("prepare/serialize(size)");
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        uint8_t splitStatus[32];
        retrieveSplitStatus<lowest_level_t,ARGS...>(splitStatus);

        size_t deltaFirstNotagOnce = 0;

    //----------------------------------------------------------------------
    ts_serialize1.stop();
    //----------------------------------------------------------------------

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    auto ts_serialize2 = statistics_.produceTimestamp("prepare/serialize(buffer)");
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        // NOTE: we may have to serialize ONLY a part of the arguments. For instance, if some are references and a previous call has been made,
        // we can afford to skip a new broadcast for these arguments.
        vector<char> buf;
        [[maybe_unused]] size_t delta = 0;

        ////////////////////////////////////////////////////////////////////////
        // Example of memory offsets for serialization over several DPUs
        //
        //   000000000011111111
        //   012345678901234567
        //
        //   AAAABBBBBBBBCCCCCC
        //   ****        ******   => args A and C must be broadcasted to all DPUs
        //   ----d0d1d2d3------   => arg B have specific information for each DPU
        //
        // Here are the memory offsets for the 3 args A,B,C over the 4 DPUs:
        //            A  B  C
        //    DPU0:  [0, 4,12]
        //    DPU1:  [0, 6,12]
        //    DPU2:  [0, 8,12]
        //    DPU3:  [0,10,12]
        //
        // In this sample, the length will be 4 (for A) + 2 (for B) + 6 (for C) = 12
        //
        // NOTE: if there is no split argument, all the DPUs will receive the same offsets.
        //
        ////////////////////////////////////////////////////////////////////////

        // rows->DPU  colums->args
        using offset_matrix_t = std::vector<std::vector< pair<size_t,size_t> >>;

        // We need a vehicle to provide information for broadcast through scatter/gather API.
        offset_matrix_t offsets (dpuSet_->getDpuNumber());
        for (auto& vec :  offsets)  { vec.resize(std::tuple_size_v<decltype(targs)> ); }

        // This implementation will return an iterable over the arguments (potentially split)
        // NOTE: the previous implementation returned a vector (was more memory consuming)
        auto transfo = [&] (size_t argIdx, auto&& arg)
        {
            // We get the type of the provided argument.
            using  type = decltype(arg);

            // We get as well its decayed version.
            using dtype = std::decay_t<type>;

            // Maybe this type is a splitter (i.e. of type SplitProxy)
            constexpr bool is_splitter = is_splitter_v<dtype>;

            // If the incoming type is a splitter, we remove the splitter encapsulation. Otherwise we provide the same type.
            using result_t = std::conditional_t <is_splitter,
                remove_splitter_t<dtype>,
                type
            >;

            // The iterable will return a pair.
            using info_t = std::pair <bool, std::remove_reference_t<result_t>>;

            DEBUG_ARCH_UPMEM ("[ArchUpmem::prepare]  transfo %3ld  is_splitter: %d \n", argIdx, is_splitter);

            // Instead of a vector (see previous version), we can return an iterable
            // -> we will have only one split part in memory at the same time.
            struct iterator
            {
                type&  arg_;  // we get a reference on the argument (no copy then)
                size_t div_;
                size_t idx_;
                size_t nb_;

                bool operator!= (const iterator & other) const { return idx_ != other.idx_; }

                iterator& operator++ () { ++idx_;  return *this; }

                info_t operator* () const
                {
                    // We use here the 'div' factor that tells us what is the level (rank or DPU)
                    // of parallelization.
                    return info_t {
                        true,  // true means that this item has to be serialized.
                        SplitOperator<std::decay_t<type>>::split (arg_, idx_/div_, nb_/div_)
                    };
                }
            };

            struct iterable_wrapper
            {
                type&  arg_;   // we get a reference on the argument (no copy then)
                size_t div_;
                size_t nb_;

                auto begin() const  { return iterator {arg_, div_,  0, nb_}; }
                auto end()   const  { return iterator {arg_, div_,nb_, nb_}; }
            };

            size_t div = splitStatus[argIdx] == Rank::LEVEL ?
                64 :  // split at rank level: each DPU of the same rank will receive the same data.
                1;    // split at DPU  level: each DPU will receive a specific data.

            return iterable_wrapper {arg, div, dpuSet_->getDpuNumber()};

        }; // end of auto transfo = []

        std::vector<size_t> sumSizePerDpu (dpuSet_->getDpuNumber(), 0);

        // We can define a callback that will be called during the serialization process.
        auto cbk = [&] (size_t idxDpu, size_t idxArg, size_t offset, size_t length)
        {
            DEBUG_ARCH_UPMEM ("[cbk] idxDpu: %8ld   idxArg: %8ld   offset: %8ld  length: %8ld   split: %d\n", idxDpu, idxArg, offset, length, splitStatus[idxArg]);

            offsets[idxDpu][idxArg] = make_pair (offset, length);

            // We update the sum of arg size for the DPU.
            sumSizePerDpu[idxDpu] += length;

            // NOTE: for the moment, we only rely on the first DPU for computing the 'once' argument delta
            if (hasTagOnce and idxDpu==0 and idxArg<=firstNoTagOnceRefIdx) {  deltaFirstNotagOnce += length; }
        };

        auto info = [&] (size_t idx, auto&& item)
        {
            return std::tuple (splitStatus[idx], splitStatus[idx]>0, dpuSet_->getDpuNumber());
        };

        bool isLoadedBinaryMatching = previousBinary_.match (bpl::core::type_shortname<TASK<arch_t>>(), 'A');

        DEBUG_ARCH_UPMEM ("[ArchUpmem::prepare]  before tuple_to_buffer:   isLoadedBinaryMatching: %d \n", isLoadedBinaryMatching);

        // We create the serialization buffer.
        if (isLoadedBinaryMatching and hasTagOnce)
        {
            // We get rid of the first arguments that are tagged with 'once'
            auto targsSliced = bpl::core::tuple_slice <firstNoTagOnceRefIdx, sizeof...(ARGS)>(targs);

            // We need to resize the offsets matrix since the number of arguments is not the same.
            for (auto& vec :  offsets)  { vec.resize(std::tuple_size_v<decltype(targsSliced)> ); }

            DEBUG_ARCH_UPMEM ("[ArchUpmem::prepare]  slicing the args... #args: %ld\n", std::tuple_size_v<decltype(targsSliced)>);

            // We serialize the (sliced) pack of arguments.
            buf   = Serializer::tuple_to_buffer (targsSliced, info, transfo, cbk);

            // We set the actual delta for calling dpu_push_sg_xfer -> this is the one computed in the previous call to 'run'.
            deltaFirstNotagOnce = firstNotagOnce_deltaBuffer_;
        }
        else
        {
            // We serialize the arguments.
            buf = Serializer::tuple_to_buffer (targs, info, transfo, cbk);

            // We keep track of the 'once' delta for further 'run' calls.
            firstNotagOnce_deltaBuffer_ = deltaFirstNotagOnce;

            // We set the actual delta for calling dpu_push_sg_xfer.
            deltaFirstNotagOnce = 0;
        }

        // We want to compute the max size of serialization for all the DPU.
        size_t maxSumSizeDpu=0;  for (auto s : sumSizePerDpu)  { if (s>maxSumSizeDpu) { maxSumSizeDpu=s; } }

        DEBUG_ARCH_UPMEM ("[ArchUpmem::prepare]  after  tuple_to_buffer  maxSumSizeDpu: %ld   deltaFirstNotagOnce: %ld \n",
            maxSumSizeDpu, deltaFirstNotagOnce
        );

        statistics_.set ("broadcast_size", buf.size());

        // We remember the size of the buffer
        __attribute__((unused)) size_t initialSize = buf.size();

        // Now, we must be sure to respect alignment constraints for broadcast -> we resize the buffer accordingly.
        buf.resize (core::roundUp<8> (buf.size()));

        DEBUG_ARCH_UPMEM ("[ArchUpmem::prepare]  serialization done,  alreadyLoaded=%d  initSize=%ld  buf.size=%ld  deltaFirstNotagOnce=%ld  delta=%ld  firstNoTagOnceRefIdx=%d \n",
            alreadyLoaded, initialSize, buf.size(), deltaFirstNotagOnce, delta, firstNoTagOnceRefIdx
        );

    //----------------------------------------------------------------------
    ts_serialize2.stop();
    //----------------------------------------------------------------------

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    auto ts_loadbinary = statistics_.produceTimestamp("prepare/loadBinary");
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        // We load the binary on the DPUs if needed, according to the task name and the size of __args__ buffer.

        if (not isLoadedBinaryMatching)
        {
            // We use the name of the type thanks to some MPL magic -> no more need to define a 'name' function in the task structure.
            alreadyLoaded = loadBinary (bpl::core::type_shortname<TASK<arch_t>>(), 'A', maxSumSizeDpu);

            DEBUG_ARCH_UPMEM ("[ArchUpmem::prepare]  alreadyLoaded: %d   maxSumSizeDpu: %ld \n",
                alreadyLoaded, maxSumSizeDpu
            );
        }

    //----------------------------------------------------------------------
    ts_loadbinary.stop();
    //----------------------------------------------------------------------

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    auto ts_broadcast = statistics_.produceTimestamp("prepare/broadcast");
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        struct data_t
        {
            vector<char>&    buffer;
            offset_matrix_t& matrix;
        };

        data_t data = { buf, offsets};

        auto get_block = [] (struct sg_block_info *out, uint32_t dpu_index, uint32_t arg_index, void *args)
        {
            data_t* data = (data_t*)args;

            offset_matrix_t& offsets = data->matrix;
            vector<char>&    buffer  = data->buffer;

            bool result = arg_index < offsets[dpu_index].size();

            if (result)
            {
                out->addr   = offsets[dpu_index][arg_index].first + (uint8_t*) buffer.data();
                out->length = offsets[dpu_index][arg_index].second;

                DEBUG_ARCH_UPMEM ("[get_block]  args: %p  dpu_index: %d  arg_index: %d   #buffer: %ld  #offsets: %ld  #offsets[dpu_index]: %ld  => %p  %d \n",
                    args, dpu_index, arg_index, buffer.size(), offsets.size(), offsets[dpu_index].size(),
                    out->addr, out->length
                );
            }

            return result;
        };

        // Apparently, we can define a lambda and use it as a field of 'get_block_t'.
        get_block_t block_info { .f= get_block, .args= &data, .args_size=sizeof(data)};

        //DPU_SG_XFER_DISABLE_LENGTH_CHECK

        // WARNING: since the total length data can be different from one DPU to another,
        // we use the flag DPU_SG_XFER_DISABLE_LENGTH_CHECK.
        DPU_ASSERT (dpu_push_sg_xfer (
            set(),
            DPU_XFER_TO_DPU,
            "__args__",
            deltaFirstNotagOnce,  // NOTE: same delta for all DPU (potential issue if different serial size per DPU ?)
            maxSumSizeDpu,
            &block_info,
            DPU_SG_XFER_DISABLE_LENGTH_CHECK
        ));

        statistics_.increment ("dpu_push_sg_xfer");

        // We are in a special case here: input parameters preparation in the context
        // of UPMEM's arch consists in broadcasting the input data to the DPUs.
        // We need to broadcast the data to each DPU of out allocated DPUs set.
        dpu_set_t dpu;
        uint32_t each_dpu;
        DPU_FOREACH (set(), dpu, each_dpu)
        {
            // We also send the dpu id
            DPU_ASSERT (dpu_broadcast_to (dpu, "__dpuid__", 0, &each_dpu, sizeof(each_dpu), DPU_XFER_DEFAULT));
            statistics_.increment ("dpu_broadcast_to");

            // We send the total number of task units
            uint32_t nbTaskUnits = getProcUnitNumber();
            DPU_ASSERT (dpu_broadcast_to (dpu, "__nbtaskunits__", 0, &nbTaskUnits, sizeof(nbTaskUnits), DPU_XFER_DEFAULT));

            // We send the "reset" status
            uint32_t resetStatus = reset_;
            DPU_ASSERT (dpu_broadcast_to (dpu, "__reset__", 0, &resetStatus, sizeof(resetStatus), DPU_XFER_DEFAULT));

            // We send the split status of each argument (up to 32 arguments). This is will be useful on the DPU side for
            // parameters whose split can be dynamically found.
            uint8_t splitStatus[32];
            retrieveSplitStatus<lowest_level_t,ARGS...>(splitStatus);
            DPU_ASSERT (dpu_broadcast_to (dpu, "__argsSplitStatus__", 0, &splitStatus, sizeof(splitStatus), DPU_XFER_DEFAULT));

        } // end of DPU_FOREACH

    //----------------------------------------------------------------------
    ts_broadcast.stop();
    //----------------------------------------------------------------------

        DEBUG_ARCH_UPMEM ("[ArchUpmem::prepare]  END\n");
    }

    const core::Statistics& getStatistics() const { return statistics_; }

private:
    core::Statistics statistics_;

    /** Handle on a set made of ranks or DPUs. */
    std::shared_ptr<details::dpu_set_handle_t> dpuSet_;
    struct dpu_set_t& set() const { return dpuSet_->handle(); }

    bool reset_ = false;

    bool isLoaded_ = false;
    bool isLoaded() const { return isLoaded_; }
    void setLoaded (bool b) { isLoaded_ = b; }

    struct BinaryInfo
    {
        std::string_view name;  char type=0;  size_t size=0;

        bool operator==(const BinaryInfo& other) const { return name==other.name and type==other.type and size==other.size; }

        bool match (std::string_view name, char type) const  {  return this->name==name and this->type==type;  }

        std::string to_string() const
        {
            std::stringstream ss;
            ss << name << '.' << type << "." << size;
            return ss.str();
        }
    };
    BinaryInfo previousBinary_;

    size_t firstNotagOnce_deltaBuffer_ = 0;

    /** \brief Load a binary into the DPUs
     * \param[in] name : name of the binary
     * \param[in] type : type of the binary
     * \return tells whether the binary was already loaded
     */
    bool loadBinary (std::string_view name, char type, size_t size)
    {
        bool alreadyLoaded = false;

        BinaryInfo currentBinary { name, type, size };

        DEBUG_ARCH_UPMEM ("[ArchUpmem::loadBinary]  isLoaded()=%d  name='%s'  (previous='%s')  type='%c'  size=%ld  eq=%d\n",
            isLoaded(), std::string(name).data(), previousBinary_.to_string().c_str(), type, size,
            (currentBinary == previousBinary_)
        );

        if (currentBinary == previousBinary_)
        {
            alreadyLoaded = true;
        }
        else
        {
            alreadyLoaded = false;

            previousBinary_ = currentBinary;

            std::string binary = look4binary (name, type, size);

            DEBUG_ARCH_UPMEM ("[ArchUpmem::loadBinary]  call to dpu_load for '%s'\n", binary.c_str());

            DPU_ASSERT(dpu_load(set(), binary.c_str(), NULL));

            statistics_.addTag ("resources/binary", binary);
            statistics_.increment ("dpu_load");
        }

        return alreadyLoaded;
    }

    /** Get the actual size, compatible of the MRAM variable buffer for '__args__' variable
     * \param[in] size: size of the input parameters to be broadcasted
     * \return the allowed size in MBytes
     */
    size_t getAllowedBufferSizeMBytes (size_t size) const
    {
        // we look for the best fit for parameters buffer size
        size_t idx=0;
        for ( ; idx<sizeof(BROADCAST_SIZE_MBYTES)/sizeof(BROADCAST_SIZE_MBYTES[0]) ; ++idx)
        {
            if (size <= 1024*1024*BROADCAST_SIZE_MBYTES[idx])  { return BROADCAST_SIZE_MBYTES[idx]; }
        }

        // If the size is too big, we return 0 by convention
        return 0;
    }

    /** \brief Look for a DPU binary file that fulfills the task name and the type
     * The directory where the binary is looked from is provided by a environment variable.
     * \param[in] taskname : name of task
     * \param[in] type : type of the binary
     * \param[in] size : size (in bytes) of the input parameters to be broadcasted to the DPUs
     * \return the file path of the binary (if found)
     */
    std::string look4binary (std::string_view taskname, char type, size_t size)
    {
        // We compute the actual size of the buffer
        size_t actualSize = getAllowedBufferSizeMBytes (size);

        std::stringstream ss;
        ss << taskname << '.' << type << "." << actualSize << "." << "dpu";
        std::string result = ss.str();

        char* d = getenv("DPU_BINARIES_DIR");
        if (d!=nullptr)
        {
            for (bpl::core::directory_entry entry : bpl::core::recursive_directory_iterator(d))
            {
                std::string filePath = entry.path().string();
                std::string last_element(filePath.substr(filePath.rfind("/") + 1));
                if (last_element == result)  { result = filePath; break; }
            }
        }

        DEBUG_ARCH_UPMEM ("[look4binary]  name='%s'  type='%c'  size=%ld  -> actualSize=%ld   file='%s' \n",
            std::string(taskname).data(), type, size, actualSize, result.c_str()
        );

        return result;
    }

    void computeCyclesStats (const std::vector<core::TimeStats>& nbCycles, uint32_t clocks_per_sec)
    {
        char buffer[256];

        auto [min,max] = core::TimeStats::minmax (nbCycles);

        statistics_.addTag ("dpu/clock", std::to_string(clocks_per_sec));

        auto dump = [&] (const std::string& prefix, auto value)
        {
            auto all = value.unserialize + value.split + value.exec + value.result;

            snprintf (buffer, sizeof(buffer), "time: %9.6f sec  (%9.6f)  [percent]  unserialize: %5.2f   split: %5.2f  exec:%5.2f  result: %5.2f",
                double(value.all) / clocks_per_sec,
                double(all)       / clocks_per_sec,
                100.0*value.unserialize / all,
                100.0*value.split       / all,
                100.0*value.exec        / all,
                100.0*value.result      / all
            );
            statistics_.addTag (prefix,buffer);
        };

        dump ("dpu/time/max", max);
        dump ("dpu/time/min", min);
    }

};

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
} };  // end of namespace
////////////////////////////////////////////////////////////////////////////////

inline auto operator""_rank    (unsigned long long int nb)  {   return bpl::arch::ArchUpmem::Rank    {nb};  }
inline auto operator""_dpu     (unsigned long long int nb)  {   return bpl::arch::ArchUpmem::DPU     {nb};  }
inline auto operator""_tasklet (unsigned long long int nb)  {   return bpl::arch::ArchUpmem::Tasklet {nb};  }

////////////////////////////////////////////////////////////////////////////////
#endif // __BPL_ARCH_UPMEM__
