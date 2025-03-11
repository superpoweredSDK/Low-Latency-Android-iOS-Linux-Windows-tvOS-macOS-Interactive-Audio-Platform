import React, { useState, useEffect } from 'react';
import { StyleSheet, Text, View, Button, Switch } from 'react-native';

import NativeSuperpoweredEngine from './specs/NativeSuperpoweredEngine';

const App = () => {

  const [isFlangerEnabled, setFlangerEnabled] = useState(true);

  const initializeSuperpoweredModule = () => {
    NativeSuperpoweredEngine.initSuperpowered();
  };

  const onPress = () => {
    NativeSuperpoweredEngine.togglePlayback();
  };

  const handleFlangerChange = () => {
    const newFlangerState = !isFlangerEnabled;
    setFlangerEnabled(newFlangerState);
    NativeSuperpoweredEngine.enableFlanger(newFlangerState);
  };

  useEffect(() => {
    initializeSuperpoweredModule();
  }, []);  // The empty dependency array ensures this useEffect runs once when the component mounts.

  return (
    <View style={styles.container}>
      <View style={styles.buttonContainer}>
        <Button title="Play / Pause" color="#0000FF" onPress={onPress} />
      </View>
      <View style={styles.switchContainer}>
        <Switch
          onValueChange={handleFlangerChange}
          value={isFlangerEnabled}
        />
        <Text style={styles.label}>Flanger Enabled</Text>
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    justifyContent: 'flex-start',
    alignItems: 'center',
    paddingTop: 50,
  },
  buttonContainer: {
    marginBottom: 20,
  },
  switchContainer: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  label: {
    marginLeft: 8,
  },
});

export default App;
