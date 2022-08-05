#pragma once
#include <iostream>
#include <sstream>
#include <thread>
#include <mutex>

namespace gauges{
    static inline void move_up(int lines) { std::cout << "\033[" << lines << "A"; }
    static inline void move_down(int lines) { std::cout << "\033[" << lines << "B"; }
    static inline void move_right(int cols) { std::cout << "\033[" << cols << "C"; }
    static inline void move_left(int cols) { std::cout << "\033[" << cols << "D"; }


    struct Gauge {
        void set_title (const std::string &nt){
            title = nt;
            need_refresh = true;
        }
        void set_status (const std::string &nt){
            status = nt;
            need_refresh = true;
        }
        void set_left_bar_char (const std::string &nt){
            left_bar_char = nt;
            need_refresh = true;
        }
        void set_right_bar_char (const std::string &nt){
            right_bar_char = nt;
            need_refresh = true;
        }
        void set_completed_work_char (const std::string &nt){
            completed_work_char = nt;
            need_refresh = true;
        }
        void set_current_progress_char (const std::string &nt){
            current_progress_char = nt;
            need_refresh = true;
        }
        void set_width (unsigned int nt){
            width = nt;
            need_refresh = true;
        }
        bool set_total_work(float nt){
            if (nt<0) return false;
            total_work = nt;
            need_refresh = true;
            return true;
        }
        void tick() {
            set_completed_work(completed_work + 1);
        }
        bool set_completed_work (float cw) {
            if (cw < 0) return false;
            if (cw > total_work) cw = total_work;
            completed_work = cw;
            if (completed_work == total_work) finished = true;
            need_refresh = true;
            return true;
        };
        void restart(){
            completed_work = 0;
            finished = false;
            need_refresh = true;
        }
        bool is_complete(){
            return finished;
        }
        void complete(){
            set_completed_work(total_work);
        }
        friend std::ostream & operator << (std::ostream & os, Gauge &g){
            if (!g.need_refresh) return os;
            std::stringstream o;
            o << g.title << g.left_bar_char;
            int current_progress = (g.completed_work / g.total_work) * (float) g.width;
            for (int i=0;i<current_progress;i++) {
                o << g.completed_work_char;
            }
            if (current_progress<g.width) o << g.current_progress_char;
            for (int i=current_progress+1;i<g.width;i++) {
                o << g.pending_work_char;
            }
            o << g.right_bar_char << g.status;
            std::string s = o.str();
            os << s;
            for (auto i=s.size();i<g.printed_char_count;i++) os << ' ';
            move_left(g.printed_char_count>s.size()?g.printed_char_count:s.size());
            g.printed_char_count = s.size();
            g.need_refresh = false;
            return os;
        };

        friend struct Gauges;

        bool need_refresh{};
    protected:
        std::string left_bar_char = "[";
        std::string right_bar_char  = "]";
        std::string completed_work_char = "=";
        std::string current_progress_char = ">";
        std::string pending_work_char = "-";
        unsigned int width = 50;
        float total_work = 100;
        float completed_work = 0;
        std::string title;
        std::string status;
        bool finished;
        unsigned int printed_char_count = 0 ;
    };

    struct Gauges {
        Gauges(unsigned int limit){
            gauges.reserve(limit);
        }
        Gauge &add_gauge(const Gauge g){
            mtx.lock();
            new_gauges ++;
            auto &ng = gauges.emplace_back(g);
            mtx.unlock();
            return ng;
        }
        Gauge &new_gauge(){
            return add_gauge(Gauge());
        }
        void auto_refresh_start(unsigned int interval){
            auto_refresh_thread = std::thread([this, interval](){
                auto_refresh = true;
                while (auto_refresh){
                    std::cout << *this;
                    std::this_thread::sleep_for(std::chrono::milliseconds(interval));
                }
            });
        }

        void auto_refresh_stop(){
            auto_refresh = false;
            if (auto_refresh_thread.joinable()) auto_refresh_thread.join();
            std::cout << *this;
        }

        friend std::ostream & operator << (std::ostream & os, Gauges &gs){
            gs.mtx.lock();
            gs.add_gauge_lines(gs.new_gauges);
            gs.new_gauges = 0;
            for (int i = 0; i < gs.gauges.size(); i++){
                auto &g = gs.gauges[i];
                if (gs.first_output || g.need_refresh){
                    gs.move_to_gauge_line(i);
                    os << g;
                }
            }
            gs.move_to_gauge_line(gs.gauges.size() - 1);
            os << std::endl;
            gs.current_gauge_line++;
            gs.first_output = false;
            gs.mtx.unlock();
            return os;
        };

        std::vector<Gauge> gauges;

    private:
        void move_to_gauge_line(unsigned int i){
            if (i==current_gauge_line) return;
            if (i < current_gauge_line) {
                move_up(current_gauge_line - i);
            } else {
                move_down(i - current_gauge_line);
            }
            current_gauge_line = i;
        }
        void add_gauge_lines(unsigned int c){
            if ( c == 0 ) return;
            move_to_gauge_line(gauges.size() - c );
            for (unsigned int i=0;i<c;i++) std::cout << std::endl;
            current_gauge_line = gauges.size();
        }
        bool first_output = true;
        unsigned int current_gauge_line{0};
        unsigned int new_gauges{};
        std::mutex mtx;
        std::thread auto_refresh_thread;
        bool auto_refresh;
    };
}