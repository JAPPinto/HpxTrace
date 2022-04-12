class event_data {
public:
  apex_event_type event_type_;
  int thread_id;
  void * data; /* generic data pointer */
  event_data() : thread_id(0), data(nullptr) {};
  virtual ~event_data() {};
};

class sample_value_event_data : public event_data {
public:
  std::string * counter_name;
  double counter_value;
  bool is_threaded;
  bool is_counter;
  sample_value_event_data(int thread_id, std::string counter_name, double counter_value, bool threaded);
  ~sample_value_event_data();
};