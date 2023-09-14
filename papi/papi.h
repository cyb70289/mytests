#include <assert.h>
#include <papi.h>
#include <initializer_list>

class papi {
 public:
  template <typename T, typename... R>
  papi(T event, R... rest_events) : m_events(1 + sizeof...(R)) {
    assert(PAPI_library_init(PAPI_VER_CURRENT) == PAPI_VER_CURRENT);
    assert(PAPI_set_domain(PAPI_DOM_ALL) == PAPI_OK);
    assert(PAPI_create_eventset(&m_event_set) == PAPI_OK);
    add_events(event, rest_events...);
  }

  void start() {
    assert(PAPI_start(m_event_set) == PAPI_OK);
  }

  template <typename... T>
  void stop(T*... out_values) {
    constexpr int n = sizeof...(T);
    long long values[n]{};
    const long long* p_value = values;
    assert(m_events == n);
    assert(PAPI_stop(m_event_set, values) == PAPI_OK);
    for (const auto p_out_value : {out_values...}) {
      *p_out_value = *p_value++;
    }
  }

 private:
  template <typename T, typename... R>
  void add_events(T event, R... rest_events) {
    add_event(event);
    add_events(rest_events...);
  }

  void add_events() {}

  void add_event(int event) {
    assert(PAPI_add_event(m_event_set, event) == PAPI_OK);
  }

  void add_event(const char* event) {
    assert(PAPI_add_named_event(m_event_set, event) == PAPI_OK);
  }

  const int m_events;
  int m_event_set{PAPI_NULL};
};
