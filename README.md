# LiquidVR&trade; SDK
<img src="doc/liquidvr-logo-black.png" width="480" height="180" />

LiquidVR&trade; provides a D3D11-based interface for applications to access the following GPU features: Async Compute, Multi-GPU Affinity, Late-Latch, and GPU-to-GPU Resource Copies<sup>[1](#liquidvr-footnote1),[2](#liquidvr-footnote2)</sup>. 

* **Async Compute** : Provides in D3D11 a subset of functionality similar to async-compute functionality in D3D12.
* **Multi-GPU Affinity** : Provides explicit multi-GPU control via ability to send D3D11 API calls to one or more GPUs via an affinity mask.
* **Late-Latch** : Provides ability to reduce input or tracking latency by reading constant data updated by the CPU after the original D3D11 calls.
* **GPU-to-GPU Resource Copies** : Provides ability to copy resources between GPUs with explicit control over synchronization.

### Installation
The LiquidVR&trade; run-time is automatically installed by the current AMD drivers. All that is needed for usage in an application is the `LiquidVR.h` header file in the `inc` folder.

### Prerequisites
* AMD Radeon&trade; GCN-based GPU (recommend R9 290 series or higher for VR)<sup>[3](#liquidvr-footnote3)</sup>
* Windows 7, Windows 8.1, or Windows 10
* Radeon Software Crimson Edition drivers or later
* Building the SDK samples requires Visual Studio 2013

### Getting Started
* A Visual Studio solution for the samples can be found in the `samples` directory.
* Additional documentation can be found in the `doc` directory.

### Notes
<a name="liquidvr-footnote1">1</a>: The LiquidVR&trade; features are available even if a VR device is not installed on a system.

<a name="liquidvr-footnote2">2</a>: LiquidVR&trade; also contains a Direct-to-Display (D2D) interface, but it is not application accessible because it is instead directly used by the head-mounted display (HMD) vendors.

<a name="liquidvr-footnote3">3</a>: All Radeon&trade; GPUs based on the Graphics Core Next (GCN) architecture support the current LiquidVR&trade; feature set. However, the recommended performance requirements for the best VR experience will vary between different HMD/content platforms. For details on recommended hardware for a particular HMD vendor, see the FAQ here: http://www.amd.com/liquidvr
