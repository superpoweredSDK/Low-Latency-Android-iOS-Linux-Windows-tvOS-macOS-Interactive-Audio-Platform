import 'package:flutter/material.dart';
import 'dart:async';
import 'package:path_provider/path_provider.dart';


import 'package:superpowered_player_with_effects/superpowered_player_with_effects.dart' as superpowered;

void main() {
  runApp(const MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  bool isChecked = true;

  @override
  void initState() {
    super.initState();
    initPlayer();
  }

  Future<void> initPlayer() async {
    superpowered.initialize((await getTemporaryDirectory()).path);
  }
  
  @override
  Widget build(BuildContext context) {
    const textStyle = TextStyle(fontSize: 25);
    const spacerSmall = SizedBox(height: 10);
    return MaterialApp(
      home: Scaffold(
        appBar: AppBar(
          title: const Text('Superpowered with Flutter example'),
        ),
        body: SingleChildScrollView(
          child: Container(
            padding: const EdgeInsets.all(10),
            child: Column(
              children: [
                spacerSmall,
                ElevatedButton(
                  child: const Text('Play / Pause'),
                  onPressed: () {
                    superpowered.togglePlayback();
                  },
                ),
                CheckboxListTile(
                  title: Text("Flanger enabled"), //    <-- label
                  value: isChecked,
                  onChanged: (bool? value) {
                    setState(() {
                    isChecked = value!;
                    superpowered.enableFlanger(value!);
                   });
                 },
              )
              ],
            ),
          ),
        ),
      ),
    );
  }
}
