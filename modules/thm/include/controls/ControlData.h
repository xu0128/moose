#pragma once

// C++ includes
#include <vector>

// Forward declarations
class ControlDataValue;
class THMControl;

/**
 * Abstract definition of a ControlData value.
 */
class ControlDataValue
{
public:
  /**
   * Constructor
   * @param name The full (unique) name for this piece of data.
   */
  ControlDataValue(std::string name) : _name(name), _declared(false), _control(nullptr) {}

  /**
   * Destructor.
   */
  virtual ~ControlDataValue() = default;

  /**
   * String identifying the type of parameter stored.
   * Must be reimplemented in derived classes.
   */
  virtual std::string type() = 0;

  /**
   * The full (unique) name of this particular piece of data.
   */
  const std::string & name() const { return _name; }

  /**
   * Get the reference to the control object that declared this control data
   */
  THMControl & getControl() const { return *_control; }

  /**
   * Set the pointer to the control object that declared this control data
   */
  void setControl(THMControl * ctrl) { _control = ctrl; }

  /**
   * Mark the data as declared
   */
  void setDeclared() { _declared = true; }

  /**
   * Get the declared state
   */
  bool getDeclared() { return _declared; }

  /**
   * Copy the current value into old (i.e. shift it "back in time")
   */
  virtual void copyValuesBack() = 0;

protected:
  /// The full (unique) name of this particular piece of data.
  const std::string _name;
  /// true if the data was declared by calling declareControlData. All data must be declared up front.
  bool _declared;
  /// The control object that declared this control data
  THMControl * _control;
};

/**
 * Concrete definition of a parameter value
 * for a specified type.
 */
template <typename T>
class ControlData : public ControlDataValue
{
public:
  /**
   * Constructor
   * @param name The full (unique) name for this piece of data.
   */
  ControlData(std::string name) : ControlDataValue(name)
  {
    _value_ptr = new T;
    _value_old_ptr = new T;
    *_value_old_ptr = 0;
  }

  virtual ~ControlData()
  {
    delete _value_ptr;
    delete _value_old_ptr;
  }

  /**
   * @returns a read-only reference to the parameter value.
   */
  const T & get() const { return *_value_ptr; }

  /**
   * @returns a read-only reference to the old value.
   */
  const T & getOld() const { return *_value_old_ptr; }

  /**
   * @returns a writable reference to the parameter value.
   */
  T & set() { return *_value_ptr; }

  /**
   * String identifying the type of parameter stored.
   */
  virtual std::string type() override;

  virtual void copyValuesBack() override;

private:
  /// Stored value.
  T * _value_ptr;
  /// Stored old value.
  T * _value_old_ptr;
};

// ------------------------------------------------------------
// ControlData<> class inline methods
template <typename T>
inline std::string
ControlData<T>::type()
{
  return typeid(T).name();
}

template <typename T>
inline void
ControlData<T>::copyValuesBack()
{
  *_value_old_ptr = *_value_ptr;
}
