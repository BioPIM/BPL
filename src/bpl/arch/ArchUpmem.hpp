////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#pragma once

#include <string>
#include <array>
#include <map>
#include <sstream>
#include <memory>
#include <numeric>
#include <any>
#include <bpl/arch/Arch.hpp>
#include <bpl/arch/ArchUpmemResources.hpp>
#include <bpl/arch/ArchUpmemMetadata.hpp>
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

#define WITH_THREADPOOL

#ifdef WITH_THREADPOOL
#include "BS_thread_pool.hpp"
#endif

////////////////////////////////////////////////////////////////////////////////
namespace bpl {
////////////////////////////////////////////////////////////////////////////////

/** Enumeration that distinguish sets made of ranks or and sets made of DPUs.
 */
enum DpuComponentKind_e {
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
// We define a type trait that checks that the incoming arguments are compatible with the task parameters.
template<typename ARG, typename PARAM> struct check_arguments: std::true_type {
};

///////////////////////////////////////////////////////////////////////////////
class ArchUpmem;

struct result_data  {
    ArchUpmem& arch;
    std::vector<uint32_t> const& heap_pointers;
    std::vector<uint32_t> const& allDPUSize;
    std::vector<uint32_t> const& allTaskletsSize;
    std::vector<uint32_t> const& allTaskletsOrder;
};

// forward declaration
template<typename T, bool Optim>
struct result_wrapper;

// [TBD] We could have some specialization of 'check_arguments', eg. avoid to use 'split' for an argument
// whose parameter is tagged with 'global'.

///////////////////////////////////////////////////////////////////////////////
/** \brief Class that provides resources for the UPMEM architecture
 *
 * \see bpl::Launcher
 */
class ArchUpmem: public ArchMulticoreResources {
public:

    // Factory that returns a type with a specific configuration.
    template<typename CFG = int> using factory = ArchUpmem;

    using config_t = void;

    /** To be improved (not really nice to have a 'kind' method that provide some typing info)
     */
    class Rank: public bpl::TaskUnit {
    public:
        using arch_t = ArchUpmem;
        using bpl::TaskUnit::TaskUnit;
        DpuComponentKind_e kind() const {
            return DpuComponentKind_e::RANK;
        }
        std::size_t getNbUnits() const {
            return getNbComponents() * 64 * NR_TASKLETS;
        }
        const char* name() const {
            return "rank";
        }
        constexpr static int LEVEL = 1;
    };

    class DPU: public bpl::TaskUnit {
    public:
        using arch_t = ArchUpmem;
        using bpl::TaskUnit::TaskUnit;
        DpuComponentKind_e kind() const {
            return DpuComponentKind_e::DPU;
        }
        std::size_t getNbUnits() const {
            return getNbComponents() * NR_TASKLETS;
        }
        const char* name() const {
            return "dpu";
        }
        constexpr static int LEVEL = 2;
    };

    class Tasklet: public TaskUnit {
    public:
        using arch_t = ArchUpmem;
        using TaskUnit::TaskUnit;
        DpuComponentKind_e kind() const {
            return DpuComponentKind_e::TASKLET;
        }
        std::size_t getNbUnits() const {
            return getNbComponents();
        }
        const char* name() const {
            return "tasklet";
        }
        constexpr static int LEVEL = 3;
    };

    // Shortcut
    using arch_t = ArchUpmem;
    //using arch_t = ArchMulticore;

    using lowest_level_t = Tasklet;

    using Serializer = Serialize<arch_t,BufferIterator<ArchMulticore>,8>;

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
    struct ArchUpmemConfiguration {
        std::shared_ptr<TaskUnit> taskunit;
        DpuComponentKind_e kind;
        std::size_t nbcomponents;
        bool trace;
        bool stats;
        bool reset;
    };

    template<typename TASKUNIT = Rank>
    static std::any make_configuration(TASKUNIT taskunit = TASKUNIT(1),
            bool trace = false, bool stats = false, bool reset = false) {
        return ArchUpmemConfiguration { std::shared_ptr < TaskUnit
                > (new TASKUNIT(taskunit)), taskunit.kind(),
                taskunit.getNbComponents(), trace, stats, reset };
    }

    ArchUpmem(std::any config) {
        try {
            auto cfg = std::any_cast < ArchUpmemConfiguration > (config);
            taskunit_ = cfg.taskunit;

            auto ts = statistics_.produceCumulTimestamp("init", "alloc");

            dpuSet_ = std::make_shared < details::dpu_set_handle_t> (cfg.kind, cfg.nbcomponents, nullptr, cfg.trace);

            useStats_ = cfg.stats;

            // We may need to reset resources between each call to 'run' (like MRAM memory management for instance).
            reset_ = cfg.reset;

            statistics_.addTag("dpu/options", dpuSet_->getOptions());

#ifdef WITH_THREADPOOL
            threadpool_ = std::make_unique<BS::thread_pool<>>(8);
#endif
        } catch (const std::bad_any_cast &e) {
            std::cout << e.what() << '\n';
        }

        // We look for binaries files.
        buildBinariesMap();
    }

    /** Constructor.
     */
    template<typename TASKUNIT = Rank>
    ArchUpmem(TASKUNIT taskunit = TASKUNIT(1), bool trace = false, bool stats =
            false, bool reset = false) :
            ArchUpmem(make_configuration(taskunit, trace, stats, reset)) {
    }

    /** Return the name of the current architecture.
     * \return the architecture name
     */
    std::string name() {
        return "upmem";
    }

    /** Return the total number of tasklets.
     * \return the number of process units
     */
    size_t getProcUnitNumber() const {
        return dpuSet_->getProcUnitNumber();
    }

    /** Return the total number of DPU.
     * \return the number of DPU
     */
    size_t getDpuNumber() const {
        return dpuSet_->getDpuNumber();
    }

    /** Return the total number of ranks.
     * \return the number of ranks
     */
    size_t getRanksNumber() const {
        return dpuSet_->getRanksNumber();
    }

    auto getProcUnitDetails() const {
        return std::make_tuple(getRanksNumber(), getDpuNumber(),
            getProcUnitNumber());
    }

    auto getTaskUnit() const {
        return taskunit_;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    DEFINE_GETTER(VECTOR_SERIALIZE_OPTIM);                                                                 \

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

        auto ts = statistics_.produceCumulTimestamp("run", "all");

        DEBUG_ARCH_UPMEM ("[ArchUpmem::run]  BEGIN\n");

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    auto ts_pre_launch = statistics_.produceCumulTimestamp("run", "pre");
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        // We determine the result type of the TASK::run method.
        // Note that the TASK type is instantiated with the current ARCH type.
        // Note also that we might have a reference as return type, so we have to remove it
        // here since we has to put the result in a vector.
        using result_t = std::remove_reference_t<bpl::return_t<decltype(&task_t::operator())>>;

        // We call the 'prepare' method that will prepare the serialization
        prepare<TASK,TRAITS...> (std::forward<ARGS>(args)...);

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    ts_pre_launch.stop();
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    auto ts_launch = statistics_.produceCumulTimestamp("run", "launch");
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        // We launch the execution.
        DPU_ASSERT(dpu_launch(set(), DPU_SYNCHRONOUS));

    //----------------------------------------------------------------------
    ts_launch.stop();
    //----------------------------------------------------------------------

        statistics_.increment ("dpu_launch");

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    auto ts_post_launch = statistics_.produceCumulTimestamp("run", "post");
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        DEBUG_ARCH_UPMEM ("[ArchUpmem::run]  dpu_launch done\n");

        // We retrieve the results, first the size then the data.
        struct dpu_set_t dpu;
        uint32_t each_dpu;

        std::vector<uint32_t> allTaskletsOrder;     allTaskletsOrder.reserve(getProcUnitNumber());
        std::vector<uint32_t> allTaskletsSize;      allTaskletsSize .reserve(getProcUnitNumber());
        std::vector<uint32_t> allDPUSize;           allDPUSize.reserve(getDpuNumber());

        std::vector<TimeStats> allNbCycles;
        uint32_t clocks_per_sec = 0;
        uint64_t totalResultSize = 0;
        uint64_t maxResultSize   = 0;

        // We first retrieve heap pointer information.
        std::vector<uint32_t> heap_pointers;

        __metadata_output__.resize (getDpuNumber());

        // We retrieve all the metadata output from the DPU.
        DPU_FOREACH(set(), dpu, each_dpu)
        {
            DPU_ASSERT (dpu_prepare_xfer(dpu, ((char*) __metadata_output__.data()) + each_dpu*sizeof(MetadataOutput)));
        }
        DPU_ASSERT(dpu_push_xfer(set(), DPU_XFER_FROM_DPU, "__metadata_output__mram__", 0, sizeof(MetadataOutput), DPU_XFER_DEFAULT));

        // We retrieve mandatory information used for unserialization
        for (const MetadataOutput& metadata : __metadata_output__)
        {
            for (auto idx : metadata.result_tasklet_order)  {
                VERBOSE_ARCH_UPMEM ("[ArchUpmem::run]  result_tasklet_order:  idx=%d\n", idx);
                allTaskletsOrder.push_back (idx);
            }

            uint32_t DPUSize = 0;
            for (auto s : metadata.result_tasklet_size)  {
                VERBOSE_ARCH_UPMEM ("[ArchUpmem::run]  result_tasklet_size :  s=%d\n", s);
                allTaskletsSize.push_back (s);
                DPUSize += s;
            }
            allDPUSize.push_back (DPUSize);
            totalResultSize += DPUSize;

            if (DPUSize > maxResultSize)  { maxResultSize = DPUSize; }

            heap_pointers.push_back (metadata.heap_pointer);

        } // end of for (const MetadataOutput& metadata : __metadata_output__)

        if (useStats_)
        {
            // We retrieve optional information used for statistics
            for (const MetadataOutput& metadata : __metadata_output__)
            {
                std::array<TimeStats,NR_TASKLETS> nbCycles;
                for (const auto& val : metadata.nb_cycles)  {
                    VERBOSE_ARCH_UPMEM ("[ArchUpmem::run]  nb_cycles:  [%u,%u,%u,%u,%u] \n",
                        val.unserialize,  val.split,  val.exec,  val.result,  val.all
                    );
                    allNbCycles.push_back (val);
                }
                statistics_.addTag ("bpl/vector/PROTO_VECTORS",std::to_string(metadata.vstats.NB_VECTORS_IN_PROTO));
                statistics_.addTag ("bpl/vector/SIZEOF",       std::to_string(metadata.vstats.SIZEOF));
                statistics_.addTag ("bpl/vector/CACHE_NB",     std::to_string(metadata.vstats.CACHE_NB));
                statistics_.addTag ("bpl/vector/MEMORY_SIZE",  std::to_string(metadata.vstats.MEMORY_SIZE));
                statistics_.addTag ("bpl/vector/CACHE_ITEMS",  std::to_string(metadata.vstats.CACHE_NB_ITEMS));
                statistics_.addTag ("bpl/vector/NBITEMS_MAX",  std::to_string(metadata.vstats.NBITEMS_MAX));
                statistics_.addTag ("bpl/memtree/BLOCK_ITEMS", std::to_string(metadata.vstats.MEMTREE_NBITEMS_PER_BLOCK));
                statistics_.addTag ("bpl/memtree/MAX_MEMORY",  std::to_string(metadata.vstats.MEMTREE_MAX_MEMORY));
                statistics_.addTag ("bpl/memtree/LEVEL_MAX",   std::to_string(metadata.vstats.MEMTREE_LEVEL_MAX));
                statistics_.addTag ("bpl/sizeof/input",        std::to_string(metadata.input_sizeof));
                statistics_.addTag ("bpl/sizeof/output",       std::to_string(metadata.output_sizeof));
                statistics_.addTag ("resources/stack_size",    std::to_string(metadata.stack_size));
                statistics_.addTag ("resources/checksum/restore",  std::to_string(metadata.restoreNbErrors));
            } // end of for (const MetadataOutput& metadata : __metadata_output__)

            clocks_per_sec =  __metadata_output__[0].clocks_per_sec;

            computeCyclesStats (allNbCycles, clocks_per_sec);
            AllocatorStats allocator_stats = __metadata_output__[0].allocator_stats;
            statistics_.addTag ("resources/MRAM/used",        std::to_string(allocator_stats.used));
            statistics_.addTag ("resources/MRAM/pos",         std::to_string(allocator_stats.pos));
            statistics_.addTag ("resources/MRAM/calls/get",   std::to_string(allocator_stats.nbCallsGet));
            statistics_.addTag ("resources/MRAM/calls/read",  std::to_string(allocator_stats.nbCallsRead));
            statistics_.addTag ("resources/nbpu",      std::to_string(getProcUnitNumber()));
        }

        // Possible check -> allTaskletsOrder should be equal to the sum from 0 to N-1 = N*(N-1)/2  (with N=getProcUnitNumber())
        DEBUG_ARCH_UPMEM ("[ArchUpmem::run]  totalResultSize=%ld  maxResultSize=%ld   #allTaskletsOrder=%ld  checksum=(%ld,%d) \n",
            totalResultSize, maxResultSize,
            allTaskletsOrder.size(),
            getProcUnitNumber()*(getProcUnitNumber()-1)/2,
            std::accumulate(allTaskletsOrder.begin(), allTaskletsOrder.end(), 0)
        );
        DEBUG_ARCH_UPMEM ("[ArchUpmem::run]  'results' filled\n");

        // we may want to dump the logs
        statistics_.dump();

        dpuSet_->dump();

    //----------------------------------------------------------------------
    ts_post_launch.stop();
    //----------------------------------------------------------------------

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    auto ts_final_result = statistics_.produceCumulTimestamp("run", "result");
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        constexpr bool VECTOR_SERIALIZE_OPTIM = get_VECTOR_SERIALIZE_OPTIM_v<typename task_t::traits_t,false>;

        // We build the result. Its type may differ according to the result_t type
        // ie. one might have some template specialization for result_wrapper.
        auto finalResult = result_wrapper<result_t,VECTOR_SERIALIZE_OPTIM>{} (
            result_data {*this,heap_pointers,allDPUSize,allTaskletsSize,allTaskletsOrder}
        );

    //----------------------------------------------------------------------
        ts_final_result.stop();
    //----------------------------------------------------------------------

        return finalResult;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    // Returns the split status of a given object.
    template<typename T>
    static constexpr int getSplitStatus (T&& t)
    {
        return ::details::GetSplitStatus<std::decay_t<T>,lowest_level_t>::value;
    }

    /** Prepare the input arguments into several configurations.
     * \param[in] args: input parameters for the task to be executed.
     * \return vector of tuples, each one holding an exec configuration
     */
    template<template<typename ...> class TASK, typename...TRAITS, typename ...ARGS>
    auto prepare (ARGS&&... args)
    {
        volatile auto ts_all = statistics_.produceCumulTimestamp("prepare","all");

        using task_t = TASK<arch_t,TRAITS...>;

        using arguments_t  = std::tuple<ARGS...>;

        DEBUG_ARCH_UPMEM ("[ArchUpmem::prepare]  BEGIN\n");

        // For each type in the 'run' prototype, retrieve the fact it is a reference or not
        //using fct_t  = decltype(bpl::GetParamType(TASK<arch_t>::run));
        //using fct_t  = decltype(bpl::GetParamType( TASK<arch_t>::operator() ) );

        using proto_t      = std::decay_t<decltype(&task_t::operator())>;
        using parameters_t = typename bpl::FunctionSignatureParser<proto_t>::args_tuple;

        static_assert (std::tuple_size_v<arguments_t> == std::tuple_size_v<parameters_t>);

        // 'firstTagOnceRefIdx' is only defined to check that potential 'once' args are the first ones.
        [[maybe_unused]] constexpr int firstTagOnceRefIdx   = bpl::first_predicate_idx_v <parameters_t, bpl::hastag_once,   true>;
        [[maybe_unused]] constexpr int firstNoTagOnceRefIdx = bpl::first_predicate_idx_v <parameters_t, bpl::hasnotag_once, false>;
        [[maybe_unused]] constexpr int hasTagOnce           = firstTagOnceRefIdx < std::tuple_size_v<parameters_t>;

        size_t deltaFirstNotagOnce = 0;
        std::vector<size_t> deltaFirstNotagOnceAllDpu (dpuSet_->getDpuNumber(), 0);

        DEBUG_ARCH_UPMEM ("[ArchUpmem::prepare]  firstTagOnceRefIdx=%d   firstNoTagOnceRefIdx=%d  hasTagOnce=%d \n",
            firstTagOnceRefIdx, firstNoTagOnceRefIdx, hasTagOnce
        );

        // We serialize the tuple holding the input parameters.
        arguments_t targs {std::forward<ARGS>(args)...};

        DEBUG_ARCH_UPMEM ("[ArchUpmem::prepare]  tuple init done\n");

        [[maybe_unused]] bool alreadyLoaded = false;

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    auto ts_serialize1 = statistics_.produceCumulTimestamp("prepare", "serialize(size)");
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        uint8_t splitStatus[32];
        retrieveSplitStatus<lowest_level_t,ARGS...>(splitStatus);

        // We have to check that provided ARGS arguments are compatible with the task parameters
        // in case we want to split one of argument.
        // For instance, we can not split a vector if the underlying parameter is a tagged with 'global'
        // Indeed on one DPU, the same vector (ie shared by the tasklets of the DPU) would hold a specific
        // part for each tasklet, which is contradictory with the fact that this vector must be common
        // and identical for each tasklet.
        static_assert (bpl::compare_tuples_v<check_arguments,arguments_t,parameters_t> == true);

    //----------------------------------------------------------------------
    ts_serialize1.stop();
    //----------------------------------------------------------------------

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    auto ts_serialize2 = statistics_.produceCumulTimestamp("prepare","serialize(buffer)");
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
        using offset_matrix_t = std::vector<std::vector< pair<uint8_t*,size_t> >>;

        // We need a vehicle to provide information for broadcast through scatter/gather API.
        offset_matrix_t offsets (dpuSet_->getDpuNumber());

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
                std::decay_t<type>
            >;

            DEBUG_ARCH_UPMEM ("[ArchUpmem::prepare]  transfo %3ld  is_splitter: %d \n", argIdx, is_splitter);

            // Instead of a vector (see previous version), we can return an iterable
            // -> we will have only one split part in memory at the same time.
            struct iterator
            {
                dtype&  arg_;  // we get a reference on the argument (no copy then)
                size_t div_;
                size_t idx_;
                size_t nb_;

                bool operator!= (const iterator & other) const { return idx_ != other.idx_; }

                iterator& operator++ () { ++idx_;  return *this; }

                // Note: use decltype(auto) in order to keep possibility to have a reference as result
                decltype(auto) operator* () const
                {
                    size_t idx   = idx_/div_;
                    size_t total = (nb_+div_-1)/div_;

                    return SplitChoice<decltype(arg_),result_t,task_t>::split_view (arg_,idx,total);
                }
            };

            struct iterable_wrapper
            {
                dtype&  arg_;   // we get a reference on the argument (no copy then)
                size_t div_;
                size_t nb_;

                auto begin() const  { return iterator {arg_, div_,  0, nb_}; }
                auto end()   const  { return iterator {arg_, div_,nb_, nb_}; }
            };

            size_t div = splitStatus[argIdx] == Rank::LEVEL ?
                64 :  // split at rank level: each DPU of the same rank will receive the same data.
                1;    // split at DPU  level: each DPU will receive a specific data.

            return iterable_wrapper { (dtype&)arg, div, dpuSet_->getDpuNumber()};

        }; // end of auto transfo = []

        std::vector<size_t> sumSizePerDpu (dpuSet_->getDpuNumber(), 0);

        // We can define a callback that will be called during the serialization process.
        auto cbk = [&] (size_t idxDpu, size_t idxArg, size_t idxBlock, uint8_t* ptr, size_t length)
        {
            VERBOSE_ARCH_UPMEM ("[cbk] idxDpu: %8ld  idxArg: %8ld  idxBlock: %8ld   ptr: %p  length: %8ld   split: %d\n",
                idxDpu, idxArg, idxBlock, ptr, length, splitStatus[idxBlock]);

            if (hasTagOnce and idxArg==firstNoTagOnceRefIdx)
            {
                if (oncePaddingPerDpu_.empty())
                {
                    // we may have to add some padding to the first 'once' args.
                    auto M = *std::max_element (deltaFirstNotagOnceAllDpu.begin(), deltaFirstNotagOnceAllDpu.end());

                    for (size_t i=0; i<offsets.size(); i++)
                    {
                        // we cumul the buffer sizes for the current ith DPU
                       size_t sumSizes=0;  for (auto x : offsets[i])  { sumSizes += x.second; }

                       oncePaddingPerDpu_.push_back (M - sumSizes);
                    }
                }
            }

            // We may need to resize the vector.
            if (offsets[idxDpu].size()<=idxBlock)  {  offsets[idxDpu].resize (idxBlock+1);  }

            if (ptr != nullptr)
            {
                offsets[idxDpu][idxBlock] = make_pair (ptr, length);
            }
            else
            {
                // If ptr is null, we reuse the information of the previous dpu for that block
                offsets[idxDpu][idxBlock] = offsets[idxDpu-1][idxBlock];

                // IMPORTANT: we must consider the actual size and not a null one
                // -> otherwise the sumSizePerDpu for this DPU will not be correct and thus maxSumSizeDpu could be wrong.
                length = offsets[idxDpu][idxBlock].second;
            }

            // We update the sum of arg size for the DPU.
            sumSizePerDpu[idxDpu] += length;

            // NOTE 1: for the moment, we only rely on the first DPU for computing the 'once' argument delta
            // NOTE 2: since an argument can be made of several blocks (e.g. vector), we have to take idxArg into account here
            if (hasTagOnce and idxArg<firstNoTagOnceRefIdx)
            {
                deltaFirstNotagOnceAllDpu[idxDpu] += length;
            }
        };

        auto info = [&] (size_t idx, auto&& item)
        {
            return std::tuple (getSplitStatus(item), getSplitStatus(item)>0, dpuSet_->getDpuNumber());
        };

        bool isLoadedBinaryMatching = previousBinary_.match (bpl::type_shortname<TASK<arch_t>>(), 'A');

        DEBUG_ARCH_UPMEM ("[ArchUpmem::prepare]  before tuple_to_buffer:   isLoadedBinaryMatching: %d  hasTagOnce: %d\n", isLoadedBinaryMatching, hasTagOnce);

        size_t broadcastSize = 0;

        // We create the serialization buffer.
        if (isLoadedBinaryMatching and hasTagOnce)
        {
            // We get rid of the first arguments that are tagged with 'once'
            auto targsSliced = bpl::tuple_slice <firstNoTagOnceRefIdx, sizeof...(ARGS)>(targs);

            // We need to resize the offsets matrix since the number of arguments is not the same.
            for (auto& vec :  offsets)  { vec.resize(std::tuple_size_v<decltype(targsSliced)> ); }

            // We serialize the (sliced) pack of arguments.
            broadcastSize = Serializer::tuple_to_buffer (targsSliced, buf, info, transfo, cbk);

            DEBUG_ARCH_UPMEM ("[ArchUpmem::prepare]  slicing the args... #args: %ld  broadcastSize: %ld \n",
                std::tuple_size_v<decltype(targsSliced)>, broadcastSize);

            // We set the actual delta for calling dpu_push_sg_xfer -> this is the one computed in the previous call to 'run'.
            deltaFirstNotagOnce = firstNotagOnce_deltaBuffer_;
        }
        else
        {
            // We serialize the arguments.
            broadcastSize = Serializer::tuple_to_buffer (targs, buf, info, transfo, cbk);

            deltaFirstNotagOnce = *std::max_element (deltaFirstNotagOnceAllDpu.begin(), deltaFirstNotagOnceAllDpu.end());

            // We keep track of the 'once' delta for further 'run' calls.
            firstNotagOnce_deltaBuffer_ = deltaFirstNotagOnce;

            // We set the actual delta for calling dpu_push_sg_xfer.
            deltaFirstNotagOnce = 0;
        }

        // We want to compute the max size of serialization for all the DPU.
        size_t maxSumSizeDpu=0;
        for (auto s : sumSizePerDpu)  { if (s>maxSumSizeDpu) { maxSumSizeDpu=s; } }

        DEBUG_ARCH_UPMEM ("[ArchUpmem::prepare]  after  tuple_to_buffer  maxSumSizeDpu: %ld   deltaFirstNotagOnce: %ld  firstNotagOnce_deltaBuffer_: %ld  broadcastSize: %ld \n",
            maxSumSizeDpu, deltaFirstNotagOnce, firstNotagOnce_deltaBuffer_, broadcastSize
        );

        for ([[maybe_unused]] auto padding : oncePaddingPerDpu_)  {  DEBUG_ARCH_UPMEM ("[ArchUpmem::prepare]  once padding: %ld \n", padding);  }

        statistics_.set ("broadcast_size", broadcastSize);

        // We remember the size of the buffer
        __attribute__((unused)) size_t initialSize = buf.size();

        // Now, we must be sure to respect alignment constraints for broadcast -> we resize the buffer accordingly.
        buf.resize (bpl::roundUp<8> (buf.size()));

        DEBUG_ARCH_UPMEM ("[ArchUpmem::prepare]  serialization done,  alreadyLoaded=%d  initSize=%ld  buf.size=%ld  deltaFirstNotagOnce=%ld  delta=%ld  firstNoTagOnceRefIdx=%d \n",
            alreadyLoaded, initialSize, buf.size(), deltaFirstNotagOnce, delta, firstNoTagOnceRefIdx
        );

    //----------------------------------------------------------------------
    ts_serialize2.stop();
    //----------------------------------------------------------------------

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    auto ts_loadbinary = statistics_.produceCumulTimestamp("prepare", "loadBinary");
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        // We load the binary on the DPUs if needed, according to the task name and the size of __args__ buffer.

        if (not isLoadedBinaryMatching)
        {
            // Tuple holding the types tagged with 'global'  (we remove tags now)
            using globalparams_t = bpl::transform_tuple_t <
                typename bpl::pack_predicate_partition_t <
                    bpl::hastag_global,
                    bpl::task_params_t<task_t>
                >::first_type,
                bpl::removetag_once,
                bpl::removetag_global
            >;

            static constexpr int sizeofGlobal = bpl::sum_sizeof (globalparams_t{});
            static_assert (sizeofGlobal < 65536);

            // We compute the % of WRAM available for tasklets (round down to a decade)
            static constexpr int wramGlobalPercent       = int ( (100.0*sizeofGlobal)/65536);

            // We use the name of the type thanks to some MPL magic -> no more need to define a 'name' function in the task structure.
            alreadyLoaded = loadBinary (bpl::type_shortname<TASK<arch_t>>(), 'A', maxSumSizeDpu, wramGlobalPercent);

            DEBUG_ARCH_UPMEM ("[ArchUpmem::prepare]  alreadyLoaded: %d   maxSumSizeDpu: %ld  wramGlobalPercent: %d  #oncePaddingPerDpu_: %ld\n",
                alreadyLoaded, maxSumSizeDpu, wramGlobalPercent, oncePaddingPerDpu_.size()
            );
        }

    //----------------------------------------------------------------------
    ts_loadbinary.stop();
    //----------------------------------------------------------------------

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    auto ts_broadcast1 = statistics_.produceCumulTimestamp("prepare","broadcast(1)");
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        struct data_t
        {
            vector<char>&    buffer;
            offset_matrix_t& matrix;
            vector<size_t>&  oncePaddingPerDpu;
            bool padding;
            std::vector<uint8_t> paddingBuf;
        };

        auto get_block = [] (struct sg_block_info *out, uint32_t dpu_index, uint32_t block_index, void *args)
        {
            data_t* data = (data_t*)args;
            offset_matrix_t& offsets = data->matrix;

            bool result = true;

            if (data->padding and dpu_index < data->oncePaddingPerDpu.size())
            {
                // By convention, the first block is used to add some potential padding (for 'once' management)
                if (block_index==0)
                {
                    out->addr   = data->paddingBuf.data();
                    out->length = data->oncePaddingPerDpu [dpu_index];
                }
                else if (block_index > 0)
                {
                    result = block_index-1 < offsets[dpu_index].size();
                    if (result)
                    {
                        out->addr   = offsets[dpu_index][block_index-1].first;
                        out->length = offsets[dpu_index][block_index-1].second;
                    }
                }
            }
            else
            {
                result = block_index < offsets[dpu_index].size();
                if (result)
                {
                    out->addr   = offsets[dpu_index][block_index].first;
                    out->length = offsets[dpu_index][block_index].second;
                }
            }

            if (result)
            {
                DEBUG_ARCH_UPMEM ("[get_block]  args: %p  dpu_index: %4d  block_index: %3d  #offsets: %ld  #offsets[dpu_index]: %ld  => %7d  %p\n",
                    args, dpu_index, block_index, offsets.size(), offsets[dpu_index].size(),
                    out->length, out->addr
                );
            }
 
            return result;
        };

        // We compute the max 'once' padding size. If 0, we take at least some bytes.
        size_t maxPaddingSize = not oncePaddingPerDpu_.empty() ?
            *std::max_element(oncePaddingPerDpu_.begin(), oncePaddingPerDpu_.end()) :
            0;
        if (maxPaddingSize==0)  { maxPaddingSize=8; }

        data_t data = {
            buf,
            offsets,
            oncePaddingPerDpu_,
            deltaFirstNotagOnce==0,
            std::vector<uint8_t> (maxPaddingSize, 0)
        };

        // Apparently, we can define a lambda and use it as a field of 'get_block_t'.
        get_block_t block_info { .f = get_block,  .args = &data, .args_size = sizeof(data) };

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

    //----------------------------------------------------------------------
    ts_broadcast1.stop();
    //----------------------------------------------------------------------

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    auto ts_broadcast2 = statistics_.produceCumulTimestamp("prepare", "broadcast(2)");
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        // We prepare a vector holding the input metadata for all DPU.
        std::vector<MetadataInput> __metadata_input__ (getDpuNumber());
        for (size_t dpuIdx=0; dpuIdx<__metadata_input__.size(); dpuIdx++)
        {
            __metadata_input__[dpuIdx] = MetadataInput (
                getProcUnitNumber(),
                dpuIdx,
                sumSizePerDpu[dpuIdx] + deltaFirstNotagOnce,  // Note: if tag 'once' is used, we have to take it into account for the buffer size
                deltaFirstNotagOnce,
                reset_,
                oncePaddingPerDpu_.empty() ? 0 : oncePaddingPerDpu_[dpuIdx],
                splitStatus
            );
        }

        {
            dpu_set_t dpu;
            uint32_t each_dpu;
            DPU_FOREACH(set(), dpu, each_dpu)
            {
                DPU_ASSERT (dpu_prepare_xfer(dpu, ((char*) __metadata_input__.data()) + each_dpu*sizeof(MetadataInput)));
            }
        }

        DPU_ASSERT(dpu_push_xfer(set(), DPU_XFER_TO_DPU, "__metadata_input__", 0, sizeof(MetadataInput), DPU_XFER_DEFAULT));

    //----------------------------------------------------------------------
    ts_broadcast2.stop();
    //----------------------------------------------------------------------

        DEBUG_ARCH_UPMEM ("[ArchUpmem::prepare]  END\n");
    }

    const bpl::Statistics& getStatistics() const { return statistics_; }

    auto resetStatistics() { statistics_={}; }

private:
    std::shared_ptr<TaskUnit> taskunit_;

    bool useStats_ = false;
    bpl::Statistics statistics_;

    /** Handle on a set made of ranks or DPUs. */
    std::shared_ptr<details::dpu_set_handle_t> dpuSet_;
    struct dpu_set_t& set() const { return dpuSet_->handle(); }

    bool reset_ = false;

    std::vector<MetadataOutput> __metadata_output__;

    std::vector<size_t> oncePaddingPerDpu_;

    bool isLoaded_ = false;
    bool isLoaded() const { return isLoaded_; }
    void setLoaded (bool b) { isLoaded_ = b; }

    // We keep a map holding [filename,path] for .dpu files in the directory (or subdir) defined
    // in the DPU_BINARIES_DIR env var.
    std::map<std::string,std::string> binariesMap_;

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

#ifdef WITH_THREADPOOL
    std::unique_ptr<BS::thread_pool<>> threadpool_;
#endif

    /** \brief Load a binary into the DPUs
     * \param[in] name : name of the binary
     * \param[in] type : type of the binary
     * \return tells whether the binary was already loaded
     */
    bool loadBinary (std::string_view name, char type, size_t size, int wramGlobalDecadePercent)
    {
        bool alreadyLoaded = false;

        BinaryInfo currentBinary { name, type, size };

        DEBUG_ARCH_UPMEM ("[ArchUpmem::loadBinary]  isLoaded()=%d  name='%s'  (previous='%s')  type='%c'  size=%ld  wramGlobalDecadePercent=%d eq=%d\n",
            isLoaded(), std::string(name).data(), previousBinary_.to_string().c_str(), type, size, wramGlobalDecadePercent,
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

            std::string binary = look4binary (name, type, size, wramGlobalDecadePercent);

            DEBUG_ARCH_UPMEM ("[ArchUpmem::loadBinary]  call to dpu_load for '%s'\n", binary.c_str());

            dpu_error_t err = dpu_load(set(), binary.c_str(), NULL);
            if (err!=DPU_OK) {
                throw std::runtime_error(std::string{"error with dpu_load for "} + binary);
            }

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

    /** */
    size_t getAllowedStackSizePercent (size_t wramGlobalPercent) const
    {
        // we look for the best fit
        for (size_t idx=0; idx<sizeof(STACK_SIZE_PERCENT)/sizeof(STACK_SIZE_PERCENT[0]) ; ++idx)
        {
            bool isok = (100-wramGlobalPercent) < STACK_SIZE_PERCENT[idx];
            if (isok)  { return idx>0 ? STACK_SIZE_PERCENT[idx-1] : 0; }
        }

        // If the size is too big, we return 0 by convention
        return 0;
    }

    /** */
    void buildBinariesMap ()
    {
        binariesMap_.clear();
        char* d = getenv("DPU_BINARIES_DIR");
        if (d!=nullptr) {
            for (bpl::directory_entry entry : bpl::recursive_directory_iterator(d))  {
                std::string filePath = entry.path().string();
                std::string last_element(filePath.substr(filePath.rfind("/") + 1));
                if (last_element.ends_with(".dpu")) {
                    binariesMap_[last_element] = filePath;
                }
            }
        }
    }

    /** \brief Look for a DPU binary file that fulfills the task name and the type
     * The directory where the binary is looked from is provided by a environment variable.
     * \param[in] taskname : name of task
     * \param[in] type : type of the binary
     * \param[in] size : size (in bytes) of the input parameters to be broadcasted to the DPUs
     * \return the file path of the binary (if found)
     */
    std::string look4binary (std::string_view taskname, char type, size_t size, int wramGlobalDecadePercent)
    {
        // We compute the actual size of the buffer
        std::stringstream ss;
        ss << taskname << '.' << type << "." << "dpu";
        std::string result = ss.str();

        auto lookup = binariesMap_.find(result);
        if (lookup != binariesMap_.end())  {
            DEBUG_ARCH_UPMEM ("[look4binary]  name='%s'  type='%c'  size=%ld  ->  file='%s' \n",
                std::string(taskname).data(), type, size, result.c_str()
            );
            return lookup->second;
        }
        throw std::runtime_error(std::string{"no found binary for taskname "} + std::string{taskname});
    }

    void computeCyclesStats (const std::vector<TimeStats>& nbCycles, uint32_t clocks_per_sec)
    {
        char buffer[512];

        auto [min,max,quantiles] = TimeStats::minmax (nbCycles);

        statistics_.addTag ("dpu/clock", std::to_string(clocks_per_sec));

        auto dump = [&] (const std::string& prefix, auto value)
        {
            double all = value.unserialize + value.split + value.exec + value.result;

            snprintf (buffer, sizeof(buffer), "time: %.4f sec  (%.4f + %.4f + %.4f + %.4f)  [percent]  unserialize: %5.2f   split: %5.2f  exec:%5.2f  result: %5.2f",
                // double(value.all) / clocks_per_sec,
                double(all)               / clocks_per_sec,
                double(value.unserialize) / clocks_per_sec,
                double(value.split)       / clocks_per_sec,
                double(value.exec)        / clocks_per_sec,
                double(value.result)      / clocks_per_sec,
                100.0*value.unserialize   / all,
                100.0*value.split         / all,
                100.0*value.exec          / all,
                100.0*value.result        / all
            );
            statistics_.addTag (prefix,buffer);
        };

        dump ("dpu/time/max", max);
        dump ("dpu/time/min", min);

        std::stringstream ss;
        snprintf (buffer, sizeof(buffer), "[%2ld]", nbCycles.size());
        ss << buffer;

        for (auto x : quantiles)
        {
            snprintf (buffer, sizeof(buffer), " %.3f", (double)x / clocks_per_sec);
            ss << buffer;
        }

        statistics_.addTag ("dpu/time/quantiles", ss.str());
    }

    // Return rank info (ptr and DPU number) of the given set.
    auto getRanksInfo (struct dpu_set_t& dpuset) const
    {
        std::vector<std::pair<struct dpu_rank_t*,size_t>> rankInfo;

        dpu_set_t rankSet;
        DPU_RANK_FOREACH (dpuset, rankSet)
        {
            for (size_t r=0; r<rankSet.list.nr_ranks; r++)
            {
                size_t idxDpu=0;
                [[maybe_unused]] struct dpu_t *dpu;
                _STRUCT_DPU_FOREACH_I(rankSet.list.ranks[r], dpu, idxDpu)  {}
                rankInfo.push_back (std::make_pair(rankSet.list.ranks[r],idxDpu));
            }
        }

        return rankInfo;
    }

    auto getDpuInfo (struct dpu_rank_t* rank)
    {
        std::vector<std::pair<struct dpu_t*, size_t>> result;
        struct dpu_t* dpu;
        size_t dpu_index = 0;
        _STRUCT_DPU_FOREACH_I(rank, dpu, dpu_index)  {  result.push_back (std::make_pair(dpu,dpu_index)); }
        return result;
    }

    template<class T, bool> friend class result_wrapper;
};

////////////////////////////////////////////////////////////////////////////////
// result_wrapper template generic case
////////////////////////////////////////////////////////////////////////////////
template<typename T, bool Optim>
struct result_wrapper  {

    using type = std::vector<T>;

    type operator() (result_data const& data)  {

        //printf ("============> NO OPTIM\n");

        // We declare the vector that will hold the results coming from the tasklets (one result per tasklet)
        type results (data.arch.getProcUnitNumber());

        // We retrieve rank information (ptr and DPU nb) for each rank of the set.
        auto rankInfo = data.arch.getRanksInfo(data.arch.set());

        // We compute the cumulative number of DPU
        std::vector<size_t> dpuPerRankCumul (rankInfo.size()+1);
        dpuPerRankCumul[0] = 0;
        for (size_t i=0; i<rankInfo.size(); i++)  {  dpuPerRankCumul[i+1] = dpuPerRankCumul[i] + rankInfo[i].second; }

        // We iterate each rank in parallel.
       data.arch.threadpool_->template submit_loop<unsigned int> (0, rankInfo.size(),  [&] (std::size_t idxRank)
       {
           // We get the current rank.
           struct dpu_rank_t* rank = rankInfo[idxRank].first;

           // We get the number of DPU for this rank.
           size_t nbDpu4rank = rankInfo[idxRank].second;

           // We retrieve the smallest MRAM start among all the DPU of the current

           // We compute the smallest MRAM pointer
           size_t minHeapPtr = std::numeric_limits<size_t>::max();
           for (size_t i=0; i<nbDpu4rank; i++)
           {
               auto val = data.heap_pointers [dpuPerRankCumul[idxRank] + i];
               if (val<minHeapPtr)  { minHeapPtr = val; }
           }

           // We compute the total size for the DPU of the current rank.
           size_t maxDpuSize = 0;
           size_t totalSize  = 0;
           for (size_t i=0; i<nbDpu4rank; i++)
           {
               size_t actualIdx = dpuPerRankCumul[idxRank] + i;

               auto val = data.allDPUSize [actualIdx];

               // we have to take into account the MRAM start delta compared to the smallest one.
               val += data.heap_pointers [actualIdx] - minHeapPtr;

               if (maxDpuSize<val)  { maxDpuSize=val; }
               totalSize +=  val;
           }

           // Since we don't use scatter/gather, we must have DPU transfer of the same size,
           // so we have to take the max size value for all the DPU of the current rank.
           totalSize = maxDpuSize * nbDpu4rank;

           // We need one buffer for un-serialization; we use only one holding the different parts for the DPU
           // which is better than having several buffers (ie. only one alloc)
           std::vector<uint8_t> buffer;
           buffer.reserve (totalSize);  // don't need a resize here, just interested in the memory buffer

           // We will need a transfer matrix.
           struct dpu_transfer_matrix matrix;

           matrix.type   = DPU_DEFAULT_XFER_MATRIX;
           matrix.offset = minHeapPtr;
           matrix.size   = maxDpuSize;

           // We must be sure that the unused ptr will be set to null (for instance if a rank doesn't use all DPUs)
           for (size_t i=0; i<MAX_NR_DPUS_PER_RANK; i++)  {  matrix.ptr[i] = nullptr;  }

           struct dpu_t *dpu;
           [[maybe_unused]] size_t idxDpu    = 0;
           size_t offsetDPU = 0;

           // We configure the transfer matrix for all the DPU of the current rank.
           _STRUCT_DPU_FOREACH_I(rank, dpu, idxDpu)
           {
               // We configure the DPU for the transfer matrix.
               dpu_transfer_matrix_add_dpu (dpu, &matrix, buffer.data() + offsetDPU);

               // We update the offset for the next DPU
               offsetDPU += maxDpuSize;

           }

           // We read the results of all DPU for the current rank.
           [[maybe_unused]] auto res = dpu_copy_from_mrams (rank, &matrix);

           // We iterate each DPU index for the current rank.
           offsetDPU = 0;
           for (size_t idxDpu=0; idxDpu<nbDpu4rank; idxDpu++)
           {
               size_t idxDpuGlobal = dpuPerRankCumul[idxRank] + idxDpu;

               // We may need to shift from the actual MRAM start
                size_t deltaMRAM = data.heap_pointers [idxDpuGlobal] - minHeapPtr;;

               size_t offsetTasklet = 0;
               for (int k=0; k<NR_TASKLETS; k++)
               {
                   // We compute the current tasklet index.
                   size_t idxTasklet = idxDpuGlobal*NR_TASKLETS + k;

                   // We get the buffer for this tasklet
                   char* buf = (char*) buffer.data() + deltaMRAM + offsetDPU + offsetTasklet;

                   // We un-serialize the current result from the incoming data
                   std::decay_t<decltype(data.arch)>::Serializer::from (buf, results [idxTasklet]);

                   // We update the offset in the buffer for the next tasklet.
                   offsetTasklet += data.allTaskletsSize[idxTasklet];
               }

               // We update the offset for the next DPU
               offsetDPU += maxDpuSize;
           }
        }).wait();

        return results;
    }
};

////////////////////////////////////////////////////////////////////////////////
// result_wrapper template specialization for vectors
////////////////////////////////////////////////////////////////////////////////
template<typename T>
struct result_wrapper<std::vector<T>, true> {

    using value_type = typename std::vector<T>::value_type;

    // We define a structure whose instantiation will provide the result of the 'run' call.
    // In this specific case, each one of the N sub-results is a vector. Instead of creating
    // N vectors, we allocate a big buffer and then create N spans that will point to the
    // correct part of the big buffer. The big buffer can be allocated once (after having
    // computed the total size of MRAM from all DPUs of all ranks) and is alive as long as
    // the 'result' instance lives (so the spans will point to valid information as long as
    // 'result' exists).
    // The 'result' API offers at least iteration (begin/end) and random access (operator[])
    struct type {
        auto begin () const { return spans->begin(); }
        auto end   () const { return spans->end();   }
        auto size  () const { return spans->size();  }
        auto operator[] (std::size_t i) const { return spans->operator[] (i); }
        std::shared_ptr<std::vector<std::span<value_type>>> spans;
        std::shared_ptr<uint8_t[]> data;
    };

    type operator() (result_data const& data) {

        type result;

        // printf ("============> OPTIM\n");

        size_t globalMinAddress = std::numeric_limits<size_t>::max();
        size_t globalMaxsize    = std::numeric_limits<size_t>::min();
        size_t totalOutputSize  = 0;

        // We first compute the total MRAM size (from all DPUs of all ranks) to be retrieved.
        size_t nbdpus = 0;
        for (auto&& m : data.arch.__metadata_output__)  {  // iterate one DPU metadata output

            auto [minVecAddr,maxVecAddr] = std::minmax_element (std::begin(m.vector_info), std::end(m.vector_info),
                [] (auto a, auto b) { return a.address<b.address; });

            auto [minVecNbitems,maxVecNbitems] = std::minmax_element (std::begin(m.vector_info), std::end(m.vector_info),
                [] (auto a, auto b) { return a.nbitems<b.nbitems; });

            // We compute the MRAM size to be retrieved for the current DPU
            size_t len = (maxVecAddr->address-minVecAddr->address) + sizeof(value_type)*maxVecNbitems->nbitems;

            // We need some alignment there.
            len = roundUp<8>(len);

            if (globalMaxsize<len)  { globalMaxsize = len; }
            if (globalMinAddress>minVecAddr->address)  { globalMinAddress = minVecAddr->address; }

            nbdpus++;
        }

        // We add a few more bytes
        totalOutputSize += nbdpus * globalMaxsize;

        // We can now allocate our big buffer.
        result.data  = std::make_shared<uint8_t[]>(totalOutputSize);
        result.spans = std::make_shared<std::vector<std::span<value_type>>>();

        data.arch.statistics_.addTag ("result/buffer", std::to_string(totalOutputSize));

        size_t idxDpu = 0;
        // Now we create the spans and make them point to the correct location in the big buffer.
        for (auto&& m :data.arch.__metadata_output__)  // DPUs iteration
        {
            auto [minVecInfo,maxVecInfo] = std::minmax_element (std::begin(m.vector_info), std::end(m.vector_info),
                [] (auto a, auto b) { return a.address<b.address; });

            for (auto e : m.vector_info) {  // tasklets iteration
                result.spans->push_back (std::span<value_type>(
                    (value_type*) (result.data.get() + globalMaxsize*idxDpu + e.address-minVecInfo->address),
                    e.nbitems
                ));
            }
            idxDpu++;
        }
        // We retrieve rank information (ptr and DPU nb) for each rank of the set.
        auto rankInfo = data.arch.getRanksInfo(data.arch.set());

        // We compute the cumulative number of DPU for all the ranks.
        std::vector<size_t> dpuPerRankCumul (rankInfo.size()+1);  // 1 + number of ranks
        dpuPerRankCumul[0] = 0;
        for (size_t i=0; i<rankInfo.size(); i++)  {  dpuPerRankCumul[i+1] = dpuPerRankCumul[i] + rankInfo[i].second; }

        // We retrieve (in //) the MRAM information from all DPUs of all ranks.
        data.arch.threadpool_->template submit_loop<unsigned int> (0, rankInfo.size(),  [&] (std::size_t idxRank)
        {
            // We retrieve a handle of the rank used in the current thread.
            struct dpu_rank_t* rank = rankInfo[idxRank].first;

            struct dpu_transfer_matrix matrix;
            for (size_t i=0; i<MAX_NR_DPUS_PER_RANK; i++)  {  matrix.ptr[i] = nullptr;  }
            matrix.type   = DPU_DEFAULT_XFER_MATRIX;
            matrix.offset = globalMinAddress;
            matrix.size   = globalMaxsize;

            size_t idxDpu = 0;
            struct dpu_t* dpu;
            uint64_t reached = 0;
            _STRUCT_DPU_FOREACH_I(rank, dpu, idxDpu) {
                // Note: we must add a (potentially variable) offset (with dpuPerRankCumul)
                // because the number of used DPUs might change between the ranks.
                reached = globalMaxsize*(dpuPerRankCumul[idxRank]+idxDpu);
                dpu_transfer_matrix_add_dpu (dpu, &matrix, result.data.get() + globalMaxsize*(dpuPerRankCumul[idxRank]+idxDpu));
                idxDpu++;
            }

            if (reached + globalMaxsize > totalOutputSize)  {
                throw std::runtime_error ("buffer overflow while retrieving MRAM contents");
            }

            // We retrieve the MRAM information from all the DPUs of the current rank.
            // As a consequence, all the result spans should now point to the correct information.
            [[maybe_unused]] auto res = dpu_copy_from_mrams (rank, &matrix);
        }).wait();

        return result;
    }
};

////////////////////////////////////////////////////////////////////////////////
};  // end of namespace
////////////////////////////////////////////////////////////////////////////////

inline auto operator""_rank(unsigned long long int nb) {
    return bpl::ArchUpmem::Rank { nb };
}
inline auto operator""_dpu(unsigned long long int nb) {
    return bpl::ArchUpmem::DPU { nb };
}
inline auto operator""_tasklet(unsigned long long int nb) {
    return bpl::ArchUpmem::Tasklet { nb };
}
