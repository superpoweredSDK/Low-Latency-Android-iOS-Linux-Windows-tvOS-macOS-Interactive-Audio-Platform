import React from 'react';
import {StyleSheet, Text, View, NativeModules, Button} from 'react-native';
const App = () => {
  const onPress = () => {
    const {SuperpoweredModule} = NativeModules;
    SuperpoweredModule.init();
  };
  return (
    <View style={styles.container}>
      <Button title="Play" color="#841584" onPress={onPress} />
      <Text> Practice !</Text>

    </View>
  );
};
const styles = StyleSheet.create({
  container: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
  },
});
export default App;