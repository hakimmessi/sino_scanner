import 'package:flutter/material.dart';
import 'package:flutter_svg/flutter_svg.dart';

import '../services/sinosecuReader.dart';

class HomeScreen extends StatelessWidget {
  const HomeScreen({Key? key}) : super(key: key);


  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.white,
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Container(
              width: 100,
              height: 100,
              child: Stack(
                alignment: Alignment.center,
                children: [
                  Center(
                    child: SvgPicture.asset(
                      'assets/icon_scan.svg',
                      width: 60,
                      height: 60,
                    ),
                  ),
                ],
              ),
            ),
            const SizedBox(height: 40),
            Container(
              decoration: BoxDecoration(
                borderRadius: BorderRadius.circular(30),
                boxShadow: const [
                  BoxShadow(
                    color: Colors.black,
                    offset: Offset(0, 4),
                    blurRadius: 0,
                  ),
                ],
              ),
              child: ElevatedButton(
                onPressed: () async {
                  int result = await SinosecuReader.initializeScanner(userId: "426911010110763248", nType: 0, sdkDirectory: ".");
                  if (result == 0) {
                    print('Scanner initialized successfully');
                  } else {
                    print('Scanner failed to initialize');
                  }
                  print('Scan document button pressed');
                },
                style: ElevatedButton.styleFrom(
                  backgroundColor: Colors.white,
                  foregroundColor: Colors.black,
                  padding: const EdgeInsets.symmetric(
                      horizontal: 30, vertical: 15),
                  shape: RoundedRectangleBorder(
                    borderRadius: BorderRadius.circular(30),
                    side: const BorderSide(color: Colors.black, width: 4),
                  ),
                  elevation: 0,
                ),
                child: const Text(
                  'scan document',
                  style: TextStyle(
                    fontSize: 18,
                    fontWeight: FontWeight.bold,
                  ),
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }
}