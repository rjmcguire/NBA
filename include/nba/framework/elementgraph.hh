#ifndef __NBA_ELEMGRAPH_HH__
#define __NBA_ELEMGRAPH_HH__

#include <nba/core/queue.hh>
#include <nba/framework/computation.hh>
#include <nba/framework/threadcontext.hh>
#include <nba/framework/task.hh>
#include <nba/element/element.hh>
#include <nba/element/packetbatch.hh>
#include <vector>
#include <map>

namespace nba {

#define ROOT_ELEMENT (nullptr)

enum ElementOffloadingActions : int {
    ELEM_OFFL_NOTHING = 0,
    ELEM_OFFL_PREPROC = 1,
    ELEM_OFFL_POSTPROC = 2,
    ELEM_OFFL_POSTPROC_FIN = 4,
};

class Element;
class OffloadTask;
class PacketBatch;

class ElementGraph {
public:
    ElementGraph(comp_thread_context *ctx);
    virtual ~ElementGraph() {}

    int count()
    {
        return elements.size();
    }

    /* Inserts the given batch/offloadtask to the internal task queue.
     * This does not execute the pipeline; call flush_tasks() for that. */
    void enqueue_batch(PacketBatch *batch, Element *start_elem, int input_port = 0);
    void enqueue_offload_task(OffloadTask *otask, Element *start_elem, int input_port = 0);

    // TODO: merge with flush_tasks()
    /* Tries to execute all pending offloaded tasks.
     * This method does not allocate/free any batches. */
    void flush_offloaded_tasks();

    /* Tries to run all delayed tasks. */
    void flush_tasks();

    /* Scan and execute schedulable elements. */
    void scan_offloadable_elements();

    bool check_preproc(OffloadableElement *oel, int dbid);
    bool check_postproc(OffloadableElement *oel, int dbid);
    bool check_postproc_all(OffloadableElement *oel);

    bool check_next_offloadable(Element *offloaded_elem);
    Element *get_first_next(Element *elem);

    /**
     * Add a new element instance to the graph.
     * If prev_elem is NULL, it becomes the root of a subtree.
     */
    int add_element(Element *new_elem);
    int link_element(Element *to_elem, int input_port,
                     Element *from_elem, int output_port);

    SchedulableElement *get_entry_point(int entry_point_idx = 0);


    /**
     * Validate the element graph.
     * Currently, this checks the writer-reader pairs of structured
     * annotations.
     */
    int validate();

    /**
     * Returns the list of schedulable elements.
     * They are executed once on every polling iteration.
     */
    const FixedRing<SchedulableElement*, nullptr>& get_schedulable_elements() const;

    /**
     * Returns the list of all elements.
     */
    const FixedRing<Element*, nullptr>& get_elements() const;

    /**
     * Free a packet batch.
     */
    void free_batch(PacketBatch *batch, bool free_pkts = true);

    /* TODO: calculate from the actual graph */
    static const int num_max_outputs = NBA_MAX_ELEM_NEXTS;
protected:
    /**
     * Used to book-keep element objects.
     */
    FixedRing<Element *, nullptr> elements;
    FixedRing<SchedulableElement *, nullptr> sched_elements;

    /**
     * Used to pass context objects when calling element handlers.
     */
    comp_thread_context *ctx;

    FixedRing<void *, nullptr> queue;
    FixedRing<OffloadTask *, nullptr> ready_tasks[NBA_MAX_COPROCESSOR_TYPES];
    //FixedRing<Task *, nullptr> delayed_batches;

private:
    /* Executes the element graph for the given batch and free it after
     * processing.  Internally it manages a queue to handle diverged paths
     * with multiple batches to multipe outputs.
     * When it needs to stop processing and wait for asynchronous events
     * (e.g., completion of offloading or release of resources), it moves
     * the batch to the delayed_batches queue. */
    void process_batch(PacketBatch *batch);
    void process_offload_task(OffloadTask *otask);

    std::map<std::pair<OffloadableElement*, int>, int> offl_actions;
    std::set<OffloadableElement*> offl_fin;

    SchedulableElement *input_elem;

    friend int io_loop(void *arg);
    friend int OffloadableElement::offload(ElementGraph *mother, OffloadTask *otask, int input_port);
    friend int OffloadableElement::offload(ElementGraph *mother, PacketBatch *in_batch, int input_port);
    friend void comp_thread_context::build_element_graph(const char *config);

};

}

#endif

// vim: ts=8 sts=4 sw=4 et
