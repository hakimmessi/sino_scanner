// path: lib/screens/home_screen.dart
import 'package:flutter/material.dart';
import 'package:flutter_svg/flutter_svg.dart';
import '../services/sinosecuReader.dart'; // Corrected import path

class HomeScreen extends StatefulWidget {
  const HomeScreen({Key? key}) : super(key: key);

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  String _statusText = "Press 'Scan Document' to start";
  bool _isScannerInitialized = false;
  Map<String, dynamic>? _recognitionResult;

  @override
  void dispose() {
    if (_isScannerInitialized) {
      SinosecuReader.releaseScanner().then((_) {
        print('[Flutter UI/HomeScreen] Scanner released on dispose.');
      }).catchError((error) {
        print('[Flutter UI/HomeScreen] Error releasing scanner on dispose: $error');
      });
    }
    super.dispose();
  }

  Future<void> _handleScanButtonPressed() async {
    if (!_isScannerInitialized) {
      await _initializeScanner();
    } else {
      // If already initialized, perhaps directly try to detect and process
      await _detectAndProcessDocument();
    }
  }

  Future<void> _initializeScanner() async {
    setState(() {
      _statusText = 'Initializing scanner...';
      _isScannerInitialized = false;
      _recognitionResult = null;
    });

    // --- Configuration for initializeScanner ---
    const String userId = "426911010110763248";

    // TODO: IMPORTANT: Verify nType.
    // For general document recognition, 0 might be a default or for business cards.
    // Refer to SDK documentation (section 3.1.1.1) for the correct value for your needs.
    // this example below are from the SDK docs,
    // 1: Authorization ID is incorrect
    // 2: Device initialization is failed
    // 3: Recognition engine initialization is failed
    // 4: Authorization files are not found
    // 5: Recognition engine is failed to load templates
    // 6: Chip reader initialization is failed
    const int nType = 0;

    // TODO: IMPORTANT: Verify sdkDirectory.
    const String sdkDirectory = "/home/kinektek/sino_scanner/libs/nativeLibs";

    int initResult = await SinosecuReader.initializeScanner(
      userId: userId,
      nType: nType,
      sdkDirectory: sdkDirectory,
    );

    if (initResult == 0) { // SDK: 0 is success for InitIDCard
      setState(() {
        _statusText = 'Scanner initialized! Ready to detect.';
        _isScannerInitialized = true;
      });
      // Optionally, automatically attempt detection after successful initialization
      //await _detectAndProcessDocument();
    } else {
      setState(() {
        _statusText = 'Scanner failed to initialize. Code: $initResult\n(Refer to SDK docs for error codes)';
        _isScannerInitialized = false;
      });
    }
  }

  Future<void> _detectAndProcessDocument() async {
    if (!_isScannerInitialized) {
      setState(() {
        _statusText = "Scanner not initialized. Please initialize first.";
      });
      return;
    }

    setState(() {
      _statusText = 'Detecting document... (Place document on scanner)';
      _recognitionResult = null;
    });

    int detectionResult = await SinosecuReader.detectDocument();
    // SDK: 1 means document placed
    // SDK: 0 means cannot detect if ID document is placed or not
    // SDK: 2 means ID document was taken out
    // SDK: 3 means detected barcode of mobile phone (AR/KR series)

    if (detectionResult == 1) {
      setState(() {
        _statusText = 'Document detected! Processing...';
      });

      Map<String, dynamic> processResult = await SinosecuReader.autoProcessDocument();
      // SDK: >0 means successfully return the main types of the document
      // Other values are error codes
      if (processResult.containsKey("mainType") && (processResult["mainType"] as int? ?? -1) > 0) {
        setState(() {
          _statusText = 'Document processed successfully!';
          _recognitionResult = processResult;
        });
      } else {
        setState(() {
          _statusText = 'Failed to process document.\nResult: $processResult';
          _recognitionResult = processResult;
        });
      }
    } else {
      String detectMsg = "Document detection status: $detectionResult. ";
      if (detectionResult == 0) detectMsg += "(Cannot detect document).";
      if (detectionResult == 2) detectMsg += "(Document taken out).";
      if (detectionResult == 3) detectMsg += "(Mobile barcode detected).";
      if (detectionResult == -1) detectMsg += "(Core engine not init).";
      setState(() {
        _statusText = detectMsg + '\nPlease place a document and try again.';
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.white,
      appBar: AppBar(
        title: const Text('Sinosecu Document Scanner'),
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
      ),
      body: Center(
        child: SingleChildScrollView( // Added for scrollability if content overflows
          padding: const EdgeInsets.all(20.0),
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Container(
                width: 100,
                height: 100,
                padding: const EdgeInsets.all(15),
                decoration: BoxDecoration(
                  color: Colors.grey[200],
                  shape: BoxShape.circle,
                ),
                child: SvgPicture.asset(
                  'assets/icon_scan.svg',
                  width: 60,
                  height: 60,
                ),
              ),
              const SizedBox(height: 30),
              Text(
                _statusText,
                textAlign: TextAlign.center,
                style: const TextStyle(fontSize: 16, height: 1.5),
              ),
              const SizedBox(height: 30),
              Container(
                decoration: BoxDecoration(
                  borderRadius: BorderRadius.circular(30),
                  boxShadow: const [
                    BoxShadow(
                      color: Colors.black38, // Softer shadow
                      offset: Offset(0, 2),
                      blurRadius: 4,
                    ),
                  ],
                ),
                child: ElevatedButton(
                  onPressed: _handleScanButtonPressed,
                  style: ElevatedButton.styleFrom(
                    backgroundColor: _isScannerInitialized ? Colors.teal : Colors.white,
                    foregroundColor: _isScannerInitialized ? Colors.white : Colors.black,
                    padding: const EdgeInsets.symmetric(horizontal: 40, vertical: 20),
                    shape: RoundedRectangleBorder(
                      borderRadius: BorderRadius.circular(30),
                      side: const BorderSide(color: Colors.black, width: 2), // Thinner border
                    ),
                    elevation: 0, // Shadow handled by container
                  ),
                  child: Text(
                    _isScannerInitialized ? 'Scan Again / Detect' : 'Initialize & Scan',
                    style: const TextStyle(
                      fontSize: 18,
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                ),
              ),
              const SizedBox(height: 20),
              if (_recognitionResult != null)
                Padding(
                  padding: const EdgeInsets.only(top: 20.0),
                  child: Card(
                    elevation: 2,
                    child: Container(
                      width: double.infinity,
                      padding: const EdgeInsets.all(15.0),
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          const Text("Recognition Result:", style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
                          const SizedBox(height: 10),
                          Text("Main Document Type: ${_recognitionResult!['mainType']}", style: const TextStyle(fontSize: 16)),
                          Text("Card Type Details: ${_recognitionResult!['cardTypeDetails']}", style: const TextStyle(fontSize: 16)),
                          if (_recognitionResult!.containsKey('error') && _recognitionResult!['error'] != null)
                            Padding(
                              padding: const EdgeInsets.only(top: 8.0),
                              child: Text("Error: ${_recognitionResult!['error']}", style: const TextStyle(color: Colors.red, fontSize: 16)),
                            ),
                          // TODO: You would parse and display more fields from the `_recognitionResult` map here
                          // based on what your C++ `autoProcessDocument` handler returns.
                          // For example, if you return specific OCR fields like 'name', 'docNumber':
                          // Text("Name: ${_recognitionResult!['nameFromOCR'] ?? 'N/A'}"),
                        ],
                      ),
                    ),
                  ),
                ),
            ],
          ),
        ),
      ),
    );
  }
}