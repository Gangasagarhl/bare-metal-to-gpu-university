// F6-30 forensic evidence: a host-only MODEL of stream capture. Like a kernel node,
// each captured node keeps a copy of the argument values it was given at capture
// time. Launching the graph again reuses those copies. (Model of the documented
// behaviour; no GPU and no CUDA calls involved.)
#include <cstdio>
#include <string>
#include <vector>

struct Node                       // one captured "kernel": data[i] = data[i] * gain
{
    std::string name;
    float* data;                  // a pointer argument: captured as an address
    float gain;                   // a value argument: captured as a value
    const float* gainPtr;         // optional: read the gain through a pointer at run time
};

struct Graph
{
    std::vector<Node> nodes;
    void launch(int n) const
    {
        for (const Node& node : nodes) {
            const float g = node.gainPtr != nullptr ? *node.gainPtr : node.gain;
            for (int i = 0; i < n; ++i) {
                node.data[i] *= g;
            }
        }
    }
};

int main()
{
    const int n = 4;
    std::vector<float> frame(static_cast<std::size_t>(n));
    float gain = 1.0f;
    const float gains[4] = {1.0f, 2.0f, 4.0f, 0.5f};

    std::printf("version 1: gain passed by value, graph captured once\n");
    Graph g1;
    g1.nodes.push_back({"applyGain", frame.data(), gain, nullptr});   // capture happens here
    for (int f = 0; f < 4; ++f) {
        gain = gains[f];                                                // host changes the gain
        frame.assign(static_cast<std::size_t>(n), 10.0f);
        g1.launch(n);
        std::printf("  frame %d: host gain %.1f, node gain %.1f, pixel %.1f (expected %.1f)\n", f,
                    gain, g1.nodes[0].gain, frame[0], 10.0f * gain);
    }

    std::printf("version 2: node parameters updated before each launch\n");
    for (int f = 0; f < 4; ++f) {
        gain = gains[f];
        g1.nodes[0].gain = gain;                                        // the update step
        frame.assign(static_cast<std::size_t>(n), 10.0f);
        g1.launch(n);
        std::printf("  frame %d: host gain %.1f, pixel %.1f (expected %.1f)\n", f, gain, frame[0],
                    10.0f * gain);
    }

    std::printf("version 3: gain read through a pointer captured once\n");
    float gainSlot = 1.0f;                                              // stands for device memory
    Graph g3;
    g3.nodes.push_back({"applyGain", frame.data(), 0.0f, &gainSlot});
    for (int f = 0; f < 4; ++f) {
        gainSlot = gains[f];                                            // copy the new value in
        frame.assign(static_cast<std::size_t>(n), 10.0f);
        g3.launch(n);
        std::printf("  frame %d: slot gain %.1f, pixel %.1f (expected %.1f)\n", f, gainSlot,
                    frame[0], 10.0f * gainSlot);
    }
    return 0;
}
