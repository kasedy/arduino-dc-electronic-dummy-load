#pragma once

template<typename ValueType, typename AccumType>
struct AverageValueCalculator { 
  AccumType accumulator = 0;
  uint16_t counter = 0;

  void addMeasurement(ValueType val) {
    accumulator += val;
    counter += 1;
  }

  ValueType calculateAverageValue() {
    ValueType avg = counter == 0 ? 0 : (accumulator / counter);
    accumulator = 0;
    counter = 0;
    return avg;
  }
};

template<typename ValueType, typename AccumType>
struct AverageValueAccumulator : public AverageValueCalculator<ValueType, AccumType> { 
  ValueType instantValue = 0;
  ValueType averageValue = 0;

  bool addMeasurement(ValueType val) {
    AverageValueCalculator<ValueType, AccumType>::addMeasurement(val);
    bool isDifferent = val != instantValue;
    instantValue = val;
    return isDifferent;
  }

  bool calculateAverageValue() {
    bool isDifferent = false;
    if (AverageValueCalculator<ValueType, AccumType>::counter >= 10) {
      ValueType avg = AverageValueCalculator<ValueType, AccumType>::calculateAverageValue();
      if (avg != averageValue) {
        isDifferent = true;
        averageValue = avg;
      }
    }
    return isDifferent;
  }

  ValueType getInstantValue() {
    return instantValue;
  }

  ValueType getAverageValue() {
    return averageValue;
  }
};
