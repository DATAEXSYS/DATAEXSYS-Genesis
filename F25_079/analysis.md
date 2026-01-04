# Project Analysis

## 1. Codebase Structure Analysis
This project is a detailed technical report (Final Year Project) written in LaTeX. The codebase is structured to generate a professional academic document.

### File Structure
*   **`main.tex`**: The central entry point for the document. It contains the logic for assembling the entire report, including the preamble, chapters, and back matter.
*   **`FastFyp.cls`**: A custom LaTeX class file that enforces the specific formatting rules of the university. It builds upon the standard `book` class.
*   **`fypbib.bib`**: A BibTeX file containing all the citations used in the report.
*   **`Figures/`**: A directory containing all the images (PNG, JPG, PDF) used in data tables, flowcharts, and results.

### LaTeX Implementation Details

#### The Build Process
The project uses a standard LaTeX build pipeline. The `main.tex` file implies the following compilation sequence:
1.  `pdflatex main.tex` (First pass to collect references and TOC)
2.  `bibtex main` (To resolve citations from `fypbib.bib`)
3.  `pdflatex main.tex` (To Apply citations)
4.  `pdflatex main.tex` (Final pass to resolve all cross-references)

#### Class Customization (`FastFyp.cls`)
The `FastFyp` class is a wrapper around the `book` class (`\LoadClass[11pt,a4paper,oneside]{book}`). It customizes:
*   **Geometry**: Sets specific margins (1 inch on all sides).
*   **Headers/Footers**: Uses `fancyhdr` to create custom headers with chapter names and page numbers.
*   **Fonts**: Uses `mathptmx` for Times New Roman style text.
*   **Titles**: Uses `titlesec` to format chapter and section headings.
*   **Code Listings**: Configures the `listings` package for C-style code blocks.

#### Document Logic (`main.tex`)
*   **Variables**: The document uses custom commands for easy maintenance of project metadata:
    ```latex
    \newcommand{\fyptitle}{Secure Data Exchange System for Ad-hoc Networks Using Blockchain-Enhanced DSR}
    \newcommand{\Studentone}{Ahmed Nasir...}
    ```
    This allows the title or author names to be changed in one place and updated everywhere.
*   **Structure**:
    *   **Front Matter**: Includes the Title Page, Anti-Plagiarism Declaration, Abstract, and TOC.
    *   **Main Matter**: The core content is divided into 8 chapters, written directly in `main.tex` (though in larger projects, these are often split into separate files).
    *   **Back Matter**: Simply includes the bibliography.

## 2. Project Architecture Analysis (The System Being Documented)
The report describes a software system named **"Secure Data Exchange System for Ad-hoc Networks Using Blockchain-Enhanced DSR"**.

### Core Problem
The project addresses security vulnerabilities in Personal Area Ad-hoc Networks (PANETs), specifically targeting:
*   **Blackhole Attacks**: Nodes dropping packets.
*   **Wormhole Attacks**: Tunneling packets to disrupt routing.
*   **Sybil Attacks**: Fake identities.
*   **Replay Attacks**: Malicious packet repetition.

### Proposed Solution: "Blockchain-Enhanced DSR"
The solution modifies the standard **Dynamic Source Routing (DSR)** protocol by adding three security layers:

1.  **PKCertChain (Identity Layer)**:
    *   A blockchain layer that acts as a decentralized Certificate Authority (CA).
    *   Manages node registration using **ECC+ECDSA** keys.
    *   Uses a **Proof-of-Work** and **Quorum-based** consensus for new node entry.

2.  **RouteLogChain (Routing & Trust Layer)**:
    *   Logs route discoveries and packet transmissions.
    *   Uses **Rolling Signatures** at each hop to ensure path integrity.
    *   Optimizes path selection using a **Multi-metric Dijkstra's Algorithm** (weighing Trust, Latency, Hops, Bandwidth).

3.  **Local Trust Diaries (Reputation System)**:
    *   A local database on each node.
    *   Uses **Bayesian Inference** (Beta-Bernoulli distribution) to calculate trust scores for neighbors based on past interactions (ACKs/NACKs).

### Implementation Details
*   **Language**: C/C++ (C23 standard).
*   **Platform**: Linux (Ubuntu 22.04 LTS).
*   **Technologies**:
    *   **Networking**: POSIX Sockets, Boost.Asio.
    *   **Crypto**: OpenSSL (ECC/ECDSA).
    *   **GUI**: Qt (for the FYP-II application "DataExSys").
*   **Status**:
    *   **FYP-I**: Completed C++ simulation demonstrating attack mitigation.
    *   **FYP-II**: Planned deployment of a Linux application on real hardware.

### Testing Results
The simulation compared the modified DSR against standard DSR:
*   **Sybil Attacks**: High detection rate due to the rigid PKCertChain registration.
*   **Blackhole/Wormhole**: Improved Packet Delivery Ratio (PDR) because the trust model effectively isolates malicious nodes.
*   **Performance**: Higher processing overhead (encryption + blockchain consensus) but acceptable for the security gains in low-mobility scenarios.
