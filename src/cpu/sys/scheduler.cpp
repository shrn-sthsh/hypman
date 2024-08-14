#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <queue>

#include <log/record.hpp>

#include "hardware/hardware.hpp"
#include "pcpu/pcpu.hpp"
#include "stat/statistics.hpp"
#include "vcpu/vcpu.hpp"

#include "scheduler.hpp"


// Minimum usage searching type threshold
static constexpr std::size_t CPU_HEAP_THRESHOLD = 1 << 10;


/**
 *  @brief vCPU pinnning to pCPU Scheduler 
 *
 *  @param current pCPU data: Collection of data about pCPUs for scheduler's 
 *                            required reallocation policies
 *  @param current vCPU data: Collection of data about vCPUs for scheduler's 
 *                            required reallocation policies
 *
 *  @details Determine the mapping of vCPUs to pCPUs resulting in the most
 *  fairness of work relative to the loads on any one pCPU. 
 *
 *  Scheduler creates a prediction of a mapping of vCPUs to pCPUs that should
 *  more equally redistribute load by greedily chosing the most busy vCPU in the
 *  set of all vCPUs yet to be mapped -- updated every iteration -- and mapping 
 *  it the currently least used pCPU -- also updated every iteration.
 *
 *  Once completed, the scheduler will determine through a disperion analysis to
 *  determine whether remapping is beneficial, and it's also beneficial enough 
 *  to outweigh the cost of executing the remapping -- the loss of perfomance
 *  from halting applications as well as loss of cache locality from flushes.
 *
 *  @return execution status code
 */
manager::status_code 
manager::scheduler
(
    libvirt::vCPU::data_t &curr_vCPU_data, 
    libvirt::pCPU::data_t &curr_pCPU_data
) noexcept
{
    // Validate vCPU and pCPU data are filled
    if (curr_vCPU_data.empty())
    {
        util::log::record
        (
            "vCPU::data_t is empty", 
            util::log::type::ERROR
        );

        return EXIT_FAILURE;
    }
    if (curr_vCPU_data.empty())
    {
        util::log::record
        (
            "pCPU::data_t is empty", 
            util::log::type::ERROR
        );

        return EXIT_FAILURE;
    }


    /******************* PRIOTITZE vCPUs BY GREATER LOADS *********************/
 
    // Sort vCPUs from greatest to least usage times
    std::sort
    (
        curr_vCPU_data.begin(), curr_vCPU_data.end(),
        [] 
        (
            const libvirt::vCPU::datum_t &datum_A, 
            const libvirt::vCPU::datum_t &datum_B
        )
        {  
            return datum_A.usage_time > datum_B.usage_time; 
        }
    );
 

    /**************** PRIOTITZE pCPUs BY LESSER RELATIVE LOADS ****************/

    // Comparator for sorting and sorted insertion
    std::function<bool (libvirt::pCPU::datum_t, libvirt::pCPU::datum_t)> 
    pCPU_usage_comparator = [] 
    (
        const libvirt::pCPU::datum_t &datum_A, 
        const libvirt::pCPU::datum_t &datum_B
    )
    {
        util::stat::ulong_t usage_A = datum_A.usage_time;
        util::stat::ulong_t usage_B = datum_A.usage_time;

        if (usage_A < usage_B)
            return true;

        if (usage_A > usage_B)
            return false;

        // for pCPUs with equal usage times, prefer pCPUs with less vCPUs 
        std::size_t number_of_vCPUs_A = datum_A.number_of_vCPUs;
        std::size_t number_of_vCPUs_B = datum_B.number_of_vCPUs;

        if (number_of_vCPUs_A < number_of_vCPUs_B)
            return true;

        if (number_of_vCPUs_A > number_of_vCPUs_B)
            return false; 

        // pCPUs with equal usage time and vCPUs
        return true;
    };


    /***************** PREDICT A BETTER vCPU to pCPU MAPPING ******************/

    // Predict a better mapping: assign vCPUs from highest to lowest 
    // usage time to pCPUs from lowest to highest usage time
    std::size_t number_of_pCPUs = curr_pCPU_data.size();
    libvirt::pCPU::data_t pred_pCPU_data(number_of_pCPUs); 
    
    // When pCPU set size greater than reasonable cache, it's faster to search 
    // with a minimum heap rather than a linear search on an array 
    const bool use_pCPU_heap = number_of_pCPUs > CPU_HEAP_THRESHOLD;
    if (use_pCPU_heap) 
    {
        std::priority_queue
        <
            libvirt::pCPU::datum_t, 
            libvirt::pCPU::data_t, 
            decltype(pCPU_usage_comparator) 
        > pred_pCPU_data_heap
        (
            pCPU_usage_comparator,
            pred_pCPU_data
        );

        // Assign each vCPU a pCPU
        for (libvirt::vCPU::datum_t &curr_vCPU_datum: curr_vCPU_data)
        {
            // Always pop from heap to get the pCPU with the lowest usage time
            libvirt::pCPU::datum_t pred_pCPU_datum = pred_pCPU_data_heap.top();
            pred_pCPU_data_heap.pop();

            // Update predicted pCPU information from assignment
            pred_pCPU_datum.usage_time += curr_vCPU_datum.usage_time;
            ++pred_pCPU_datum.number_of_vCPUs;
            
            // Save which pCPU to pin
            curr_vCPU_datum.pCPU_rank = pred_pCPU_datum.pCPU_rank; 

            // Place back into heap
            pred_pCPU_data_heap.push(pred_pCPU_datum);
        }

        // Place assingments back into
        for (libvirt::pCPU::datum_t &pred_pCPU_datum: pred_pCPU_data)
        {
            pred_pCPU_datum = std::move(pred_pCPU_data_heap.top());
            pred_pCPU_data_heap.pop();
        }
    } 

    // Use linear search for minimum element otherwise
    else 
    {
        // Assign each vCPU a pCPU
        for (libvirt::vCPU::datum_t &curr_vCPU_datum: curr_vCPU_data)
        {
            // Always linear search the pCPU with the lowest usage time
            libvirt::pCPU::datum_t &pred_pCPU_datum = *std::min_element
            (
                pred_pCPU_data.begin(), 
                pred_pCPU_data.end(), 
                pCPU_usage_comparator
            );

            // Update predicted pCPU information from assignment
            pred_pCPU_datum.usage_time += curr_vCPU_datum.usage_time;
            ++pred_pCPU_datum.number_of_vCPUs;

            // Save which pCPU to pin
            curr_vCPU_datum.pCPU_rank = pred_pCPU_datum.pCPU_rank; 
        }
    }


    /******************** ALIGN PREDICTION FOR COMPARISON *********************/

    // Sort back predicted pCPU data into rank order
    std::sort
    (
        pred_pCPU_data.begin(), pred_pCPU_data.end(),
        [] 
        (
            const libvirt::pCPU::datum_t &datum_A, 
            const libvirt::pCPU::datum_t &datum_B
        )
        {
            return datum_A.pCPU_rank < datum_B.pCPU_rank;
        }
    );


    /************* DETERMINE WHETHER PREDICTION IMPROVES STATE ****************/

    // Estimate if prediction will likely perform better
    const bool favorable_to_redistribute = manager::analyze_prediction
    (
        curr_pCPU_data, 
        pred_pCPU_data
    );
    if (!favorable_to_redistribute)
    {
        util::log::record
        (
            "Did not remap vCPUs to pCPUs as predicted mapping was estimated "
            "to likely be unfavorable"
        );

        return EXIT_SUCCESS;
    }


    /**************** APPLY MAPPING DESCRIBED BY PREDICTION *******************/

    // Execute remapping of vCPUs to pCPUs as per prediction
    libvirt::status_code status;
    for (const libvirt::vCPU::datum_t &datum: curr_vCPU_data)
    {
        status = libvirt::hardware::map(datum, number_of_pCPUs);
        util::log::record
        (
            "Error incurred while remapping vCPUs to pCPUs; will continue "
            "with remaining vCPUs"
        );
    }

    return EXIT_SUCCESS;
}


/**
 *  @brief Prediction Perfomance Analyzer
 *
 *  @param current pCPU data:   Collection of current data about pCPUs for 
 *                              scheduler's required reallocation policies
 *  @param predicted pCPU data: Collection of predicted data about vCPUs for 
 *                              scheduler's required reallocation policies
 *
 *  @details Determine using despersion analysis whether the new mapping of 
 *  vCPUs to pCPUs is likely perform better than the current mapping.
 *
 *  @return whether or not to apply remapping
 */
bool
manager::analyze_prediction
(
    const libvirt::pCPU::data_t &curr_data, 
    const libvirt::pCPU::data_t &pred_data
) noexcept
{
    // Current pCPU statistics
    const auto [curr_mean, curr_devation] 
        = libvirt::pCPU::stat::mean_and_deviation(curr_data);
    std::double_t curr_dispersion = curr_devation / curr_mean;

    // Predicted pCPU statistics
    const auto [pred_mean, pred_devation] 
        = libvirt::pCPU::stat::mean_and_deviation(pred_data);
    std::double_t pred_dispersion = pred_devation / pred_mean;

    // Redistribute conditions to be true
    bool curr_pinning_high_dispersion = 
        curr_dispersion  > manager::DISPERSION_UPPER_BOUND;
        
    bool pred_pinning_low_dispersion = 
        pred_dispersion <= manager::DISPERSION_LOWER_BOUND;

    return curr_pinning_high_dispersion && pred_pinning_low_dispersion;
}
