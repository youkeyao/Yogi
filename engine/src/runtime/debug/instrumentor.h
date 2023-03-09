#pragma once

#ifdef YG_PROFILE
    namespace Yogi {

        using FloatingPointMicroseconds = std::chrono::duration<double, std::micro>;

        struct ProfileResult
        {
            std::string name;

            FloatingPointMicroseconds start;
            std::chrono::microseconds elapsed_time;
            std::thread::id thread_id;
        };

        struct InstrumentationSession
        {
            std::string name;
        };

        class Instrumentor
        {
        public:
            Instrumentor(const Instrumentor&) = delete;
            Instrumentor(Instrumentor&&) = delete;

            void begin_session(const std::string& name, const std::string& filepath = "results.json")
            {
                std::lock_guard lock(m_mutex);
                if (m_current_session) {
                    YG_CORE_ERROR("Instrumentor::BeginSession('{0}') when session '{1}' already open.", name, m_current_session->name);
                    internal_end_session();
                }
                m_output_stream.open(filepath);

                if (m_output_stream.is_open()) {
                    m_current_session = new InstrumentationSession({name});
                    write_header();
                }
                else {
                    YG_CORE_ERROR("Instrumentor could not open results file '{0}'.", filepath);
                }
            }

            void end_session()
            {
                std::lock_guard lock(m_mutex);
                internal_end_session();
            }

            void write_profile(const ProfileResult& result)
            {
                std::stringstream json;

                json << std::setprecision(3) << std::fixed;
                json << ",{";
                json << "\"cat\":\"function\",";
                json << "\"dur\":" << (result.elapsed_time.count()) << ',';
                json << "\"name\":\"" << result.name << "\",";
                json << "\"ph\":\"X\",";
                json << "\"pid\":0,";
                json << "\"tid\":" << result.thread_id << ",";
                json << "\"ts\":" << result.start.count();
                json << "}";

                std::lock_guard lock(m_mutex);
                if (m_current_session) {
                    m_output_stream << json.str();
                    m_output_stream.flush();
                }
            }

            static Instrumentor& get()
            {
                static Instrumentor instance;
                return instance;
            }
        private:
            Instrumentor() : m_current_session(nullptr)
            {
            }

            ~Instrumentor()
            {
                end_session();
            }        

            void write_header()
            {
                m_output_stream << "{\"otherData\": {},\"traceEvents\":[{}";
                m_output_stream.flush();
            }

            void write_footer()
            {
                m_output_stream << "]}";
                m_output_stream.flush();
            }

            void internal_end_session()
            {
                if (m_current_session) {
                    write_footer();
                    m_output_stream.close();
                    delete m_current_session;
                    m_current_session = nullptr;
                }
            }
        private:
            std::mutex m_mutex;
            InstrumentationSession* m_current_session;
            std::ofstream m_output_stream;
        };

        class InstrumentationTimer
        {
        public:
            InstrumentationTimer(const char* name) : m_name(name), m_stopped(false)
            {
                m_start_timepoint = std::chrono::steady_clock::now();
            }

            ~InstrumentationTimer()
            {
                if (!m_stopped)
                    Stop();
            }

            void Stop()
            {
                auto end_timepoint = std::chrono::steady_clock::now();
                auto highRes_start = FloatingPointMicroseconds{ m_start_timepoint.time_since_epoch() };
                auto elapsed_time = std::chrono::time_point_cast<std::chrono::microseconds>(end_timepoint).time_since_epoch() - std::chrono::time_point_cast<std::chrono::microseconds>(m_start_timepoint).time_since_epoch();

                Instrumentor::get().write_profile({ m_name, highRes_start, elapsed_time, std::this_thread::get_id() });

                m_stopped = true;
            }
        private:
            const char* m_name;
            std::chrono::time_point<std::chrono::steady_clock> m_start_timepoint;
            bool m_stopped;
        };

        namespace InstrumentorUtils {

            template <size_t N>
            struct ChangeResult
            {
                char data[N];
            };

            template <size_t N, size_t K>
            constexpr auto cleanup_output_string(const char(&expr)[N], const char(&remove)[K])
            {
                ChangeResult<N> result = {};

                size_t src_index = 0;
                size_t dst_index = 0;
                while (src_index < N)
                {
                    size_t match_index = 0;
                    while (match_index < K - 1 && src_index + match_index < N - 1 && expr[src_index + match_index] == remove[match_index])
                        match_index++;
                    if (match_index == K - 1)
                        src_index += match_index;
                    result.Data[dst_index++] = expr[src_index] == '"' ? '\'' : expr[src_index];
                    src_index++;
                }
                return result;
            }
        }
    }

    // Resolve which function signature macro will be used. Note that this only
    // is resolved when the (pre)compiler starts, so the syntax highlighting
    // could mark the wrong one in your editor!
    #if defined(__GNUC__) || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) || (defined(__ICC) && (__ICC >= 600)) || defined(__ghs__)
        #define YG_FUNC_SIG __PRETTY_FUNCTION__
    #elif defined(__DMC__) && (__DMC__ >= 0x810)
        #define YG_FUNC_SIG __PRETTY_FUNCTION__
    #elif (defined(__FUNCSIG__) || (_MSC_VER))
        #define YG_FUNC_SIG __FUNCSIG__
    #elif (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 600)) || (defined(__IBMCPP__) && (__IBMCPP__ >= 500))
        #define YG_FUNC_SIG __FUNCTION__
    #elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)
        #define YG_FUNC_SIG __FUNC__
    #elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)
        #define YG_FUNC_SIG __func__
    #elif defined(__cplusplus) && (__cplusplus >= 201103)
        #define YG_FUNC_SIG __func__
    #else
        #define YG_FUNC_SIG "YG_FUNC_SIG unknown!"
    #endif

    #define YG_PROFILE_BEGIN_SESSION(name, filepath) ::Yogi::Instrumentor::get().begin_session(name, filepath)
    #define YG_PROFILE_END_SESSION() ::Yogi::Instrumentor::get().end_session()
    #define YG_PROFILE_SCOPE(name) ::Yogi::InstrumentationTimer timer##__LINE__(name);
    #define YG_PROFILE_FUNCTION() YG_PROFILE_SCOPE(YG_FUNC_SIG)
#else
    #define YG_PROFILE_BEGIN_SESSION(name, filepath)
    #define YG_PROFILE_END_SESSION()
    #define YG_PROFILE_SCOPE(name)
    #define YG_PROFILE_FUNCTION()
#endif