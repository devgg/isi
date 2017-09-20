#include "frequency.hpp"
#include "timer.hpp"
#include "helpers.hpp"
#include <iostream>
#include <queue>

namespace frequency {


    template<typename PqElement>
    void add_pq_element(std::priority_queue<PqElement, std::vector<PqElement>, std::function<bool(const PqElement&, const PqElement&)>>& pq, std::ifstream* ifs) {
        if (ifs && ifs->peek() != EOF) {
            pq.push(PqElement(ifs));
        }
    }

    template<typename PqElement>
    void process(std::vector<std::ifstream>& ifstreams, const boost::filesystem::path& out_file) {
        std::priority_queue<PqElement, std::vector<PqElement>, std::function<bool(const PqElement&, const PqElement&)>> pq(PqElement::comp);
        std::ofstream ofs (out_file.string(), std::ios::out | std::ios::binary);

        for(auto& ifs: ifstreams) {
            add_pq_element(pq, &ifs);
        }

        auto pq_element = pq.top();
        pq.pop();
        uint64_t kmer = pq_element.kmer();
        uint32_t count = pq_element.count();
        add_pq_element(pq, pq_element.ifs());

        while(!pq.empty()) {
            pq_element = pq.top();
            pq.pop();
            if (pq_element.kmer() == kmer) {
                count += pq_element.count();
            } else {
                ofs.write(reinterpret_cast<const char*>(&kmer), 8);
                ofs.write(reinterpret_cast<const char*>(&count), 4);
                kmer = pq_element.kmer();
                count = pq_element.count();
            }
            add_pq_element(pq, pq_element.ifs());
        }
        ofs.write(reinterpret_cast<const char*>(&kmer), 8);
        ofs.write(reinterpret_cast<const char*>(&count), 4);
    }

    template<typename PqElement>
    void process_all_in_directory(const boost::filesystem::path& in_dir, const boost::filesystem::path& out_dir) {
        timer t = {"process"};
        t.start();

        boost::filesystem::create_directories(out_dir);
        boost::filesystem::recursive_directory_iterator it(in_dir), end;
        std::vector<boost::filesystem::path> paths;
        std::copy(it, end, std::back_inserter(paths));
        std::sort(paths.begin(), paths.end());

        std::vector<std::ifstream> ifstreams;
        for (size_t i = 0; i < paths.size(); i++) {
            if(boost::filesystem::is_regular_file(paths[i]) && paths[i].extension() == PqElement::file_extension()) {
                ifstreams.emplace_back(std::ifstream(paths[i].string(), std::ios::in | std::ios::binary));
            }
            if (ifstreams.size() == 50 || i + 1 == paths.size()) {
                process<PqElement>(ifstreams, out_dir / "1.f");
                ifstreams.clear();
            }
        }

        t.end();
        std::cout << t;
    }

}