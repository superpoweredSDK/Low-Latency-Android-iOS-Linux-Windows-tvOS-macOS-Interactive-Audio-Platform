import React, { useState, useEffect } from 'react';
import { StyleSheet, Text, View, NativeModules, Button, Switch } from 'react-native';

const App = () => {

  const { SuperpoweredModule } = NativeModules;
  const [isFlangerEnabled, setFlangerEnabled] = useState(true);

  const initializeSuperpoweredModule = () => {
    SuperpoweredModule.initSuperpowered();
  };

  const onPress = () => {
    SuperpoweredModule.togglePlayback();
  };

  const handleFlangerChange = () => {
    const newFlangerState = !isFlangerEnabled;
    setFlangerEnabled(newFlangerState);
    SuperpoweredModule.enableFlanger(newFlangerState);
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
