import 'package:flutter/material.dart';
import 'package:flutter_svg/flutter_svg.dart';
import '../services/sinosecuReader.dart';

class HomeScreen extends StatefulWidget {
  const HomeScreen({Key? key}) : super(key: key);

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  String _statusText = "Press 'Initialize Scanner' to start";
  bool _isScannerInitialized = false;
  bool _isScanning = false;
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

  Future<void> _initializeScanner() async {
    setState(() {
      _statusText = 'Initializing scanner...';
      _isScannerInitialized = false;
      _recognitionResult = null;
    });

    const String userId = "426911010110763248";
    const int nType = 0;
    const String sdkDirectory = "/home/kinektek/sino_scanner/build/linux/arm64/release/bundle/lib";

    try {
      int initResult = await SinosecuReader.initializeScanner(
        userId: userId,
        nType: nType,
        sdkDirectory: sdkDirectory,
      );

      if (initResult == 0) {
        setState(() {
          _statusText = 'Scanner initialized successfully!\nReady to scan documents.';
          _isScannerInitialized = true;
        });
      } else {
        String errorMsg = _getInitErrorMessage(initResult);
        setState(() {
          _statusText = 'Initialization failed!\nError $initResult: $errorMsg';
          _isScannerInitialized = false;
        });
      }
    } catch (e) {
      setState(() {
        _statusText = 'Initialization error: $e';
        _isScannerInitialized = false;
      });
    }
  }

  String _getInitErrorMessage(int errorCode) {
    switch (errorCode) {
      case 1: return "Authorization ID incorrect - check license";
      case 2: return "Device initialization failed - check connection";
      case 3: return "Recognition engine init failed - check SDK files";
      case 4: return "Authorization files not found";
      case 5: return "Failed to load templates";
      case 6: return "Chip reader initialization failed";
      default: return "Unknown error";
    }
  }

  Future<void> _scanDocument() async {
    if (!_isScannerInitialized) {
      setState(() {
        _statusText = "Please initialize scanner first";
      });
      return;
    }

    setState(() {
      _isScanning = true;
      _statusText = 'Place your document on the scanner...\nWaiting for detection...';
      _recognitionResult = null;
    });

    try {
      // The scanning workflow
      Map<String, dynamic> result = await SinosecuReader.scanDocumentComplete(20); // 30 second timeout

      if (result.containsKey('error')) {
        setState(() {
          _statusText = 'Scan failed: ${result['error']}';
          _isScanning = false;
        });
      } else {
        setState(() {
          _statusText = 'Document scanned successfully!';
          _recognitionResult = result;
          _isScanning = false;
        });
      }

    } catch (e) {
      setState(() {
        _statusText = 'Scan error: $e';
        _isScanning = false;
      });
    }
  }

  /*Future<void> _manualDetectAndProcess() async {
    if (!_isScannerInitialized) {
      setState(() {
        _statusText = "Please initialize scanner first";
      });
      return;
    }

    setState(() {
      _statusText = 'Checking for document...';
      _recognitionResult = null;
    });

    try {
      // Method 2: Manual step-by-step process
      int detectionResult = await SinosecuReader.detectDocument();

      String detectMsg = _getDetectionMessage(detectionResult);

      if (detectionResult == 1) {
        setState(() {
          _statusText = 'Document detected! Processing...';
        });

        Map<String, dynamic> processResult = await SinosecuReader.autoProcessDocument();

        if (processResult.containsKey("status") && (processResult["status"] as int? ?? -1) > 0) {
          // Get document fields
          Map<String, dynamic> ocrFields = await SinosecuReader.getDocumentFields(1); // OCR
          Map<String, dynamic> chipFields = await SinosecuReader.getDocumentFields(0); // Chip

          setState(() {
            _statusText = 'Document processed successfully!';
            _recognitionResult = {
              ...processResult,
              'ocr_fields': ocrFields,
              'chip_fields': chipFields,
            };
          });
        } else {
          setState(() {
            _statusText = 'Processing failed: ${processResult["status"]}';
            _recognitionResult = processResult;
          });
        }
      } else {
        setState(() {
          _statusText = detectMsg;
        });
      }
    } catch (e) {
      setState(() {
        _statusText = 'Detection error: $e';
      });
    }
  }

  String _getDetectionMessage(int code) {
    switch (code) {
      case -1: return "Core engine not initialized";
      case 0: return "No document detected\nPlace document and try again";
      case 1: return "Document detected!";
      case 2: return "Document removed";
      case 3: return "Mobile barcode detected";
      default: return "Unknown detection status: $code";
    }
  } */

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.white,
      appBar: AppBar(
        title: const Text('Sinosecu Document Scanner'),
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
      ),
      body: Center(
        child: SingleChildScrollView(
          padding: const EdgeInsets.all(20.0),
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Container(
                width: 100,
                height: 100,
                padding: const EdgeInsets.all(15),
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

              // Initialize Button
              if (!_isScannerInitialized) ...[
                ElevatedButton(
                  onPressed: _initializeScanner,
                  style: ElevatedButton.styleFrom(
                    backgroundColor: Colors.white,
                    foregroundColor: _isScannerInitialized ? Colors.white : Colors.black87,
                    padding: const EdgeInsets.symmetric(horizontal: 40, vertical: 20),
                    shape: RoundedRectangleBorder(
                      borderRadius: BorderRadius.circular(30),
                      side: const BorderSide(color: Colors.black87, width: 2), // Thinner border
                    ),
                    elevation: 0, // Shadow handled by container
                  ),
                  child: const Text('Initialize Scanner', style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold)),
                ),
              ],

              // Scan Buttons (only show when initialized)
              if (_isScannerInitialized) ...[
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                  children: [
                    Expanded(
                      child: Container(
                        margin: const EdgeInsets.only(right: 8),
                        child: ElevatedButton(
                          onPressed: _isScanning ? null : _scanDocument,
                          style: ElevatedButton.styleFrom(
                            backgroundColor: Colors.black87,
                            foregroundColor: Colors.white,
                            padding: const EdgeInsets.symmetric(horizontal: 40, vertical: 20),
                            shape: RoundedRectangleBorder(
                              borderRadius: BorderRadius.circular(30),
                              side: const BorderSide(color: Colors.black87, width: 2), // Thinner border
                            ),
                            elevation: 0,
                          ),
                          child: _isScanning
                              ? const SizedBox(
                            height: 20,
                            width: 20,
                            child: CircularProgressIndicator(color: Colors.white70, strokeWidth: 2),
                          )
                              : const Text('Auto Scan', style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold)),
                        ),
                      ),
                    ),
                    /*Expanded(
                      child: Container(
                        margin: const EdgeInsets.only(left: 8),
                        child: ElevatedButton(
                          onPressed: _isScanning ? null : _manualDetectAndProcess,
                          style: ElevatedButton.styleFrom(
                            backgroundColor: Colors.orange,
                            foregroundColor: Colors.white,
                            padding: const EdgeInsets.symmetric(vertical: 15),
                            shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(25)),
                          ),
                          child: const Text('Manual Detect', style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold)),
                        ),
                      ),
                    ), */
                  ],
                ),
                const SizedBox(height: 15),

                // Reset button
                ElevatedButton(
                  onPressed: () {
                    setState(() {
                      _statusText = "Scanner ready";
                      _recognitionResult = null;
                      _isScanning = false;
                    });
                  },
                  style: ElevatedButton.styleFrom(
                    backgroundColor: Colors.grey[600],
                    foregroundColor: Colors.white,
                    padding: const EdgeInsets.symmetric(horizontal: 30, vertical: 10),
                    shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(20)),
                  ),
                  child: const Text('Clear Results'),
                ),
              ],

              const SizedBox(height: 20),

              // Recognition Results Display
              if (_recognitionResult != null) ...[
                Card(
                  elevation: 4,
                  margin: const EdgeInsets.symmetric(vertical: 10),
                  child: Container(
                    width: double.infinity,
                    padding: const EdgeInsets.all(20.0),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Row(
                          children: [
                            Icon(Icons.check_circle, color: Colors.green, size: 24),
                            const SizedBox(width: 8),
                            const Text(
                              "Recognition Results",
                              style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold, color: Colors.green),
                            ),
                          ],
                        ),
                        const Divider(height: 20),

                        // Document Type Info
                        if (_recognitionResult!.containsKey('document_type'))
                          _buildResultRow("Document Type", _recognitionResult!['document_type']),
                        if (_recognitionResult!.containsKey('main_type'))
                          _buildResultRow("Main Type ID", _recognitionResult!['main_type']),
                        if (_recognitionResult!.containsKey('card_type'))
                          _buildResultRow("Card Type", _getCardTypeDescription(_recognitionResult!['card_type'])),

                        const SizedBox(height: 10),

                        // OCR Fields
                        if (_recognitionResult!.containsKey('ocr_fields') || _hasOcrFields()) ...[
                          const Text("OCR Data:", style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold)),
                          const SizedBox(height: 5),
                          ..._buildOcrFields(),
                        ],

                        // Chip Fields
                        if (_recognitionResult!.containsKey('chip_fields') || _hasChipFields()) ...[
                          const SizedBox(height: 10),
                          const Text("Chip Data:", style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold)),
                          const SizedBox(height: 5),
                          ..._buildChipFields(),
                        ],

                        // Legacy format support
                        if (_recognitionResult!.containsKey('status'))
                          _buildResultRow("Process Status", _recognitionResult!['status'].toString()),
                        if (_recognitionResult!.containsKey('cardType'))
                          _buildResultRow("Card Type Details", _recognitionResult!['cardType'].toString()),

                        // Error handling
                        if (_recognitionResult!.containsKey('error')) ...[
                          const SizedBox(height: 10),
                          Container(
                            padding: const EdgeInsets.all(10),
                            decoration: BoxDecoration(
                              color: Colors.red[50],
                              borderRadius: BorderRadius.circular(8),
                              border: Border.all(color: Colors.red[200]!),
                            ),
                            child: Row(
                              children: [
                                Icon(Icons.error, color: Colors.red, size: 20),
                                const SizedBox(width: 8),
                                Expanded(
                                  child: Text(
                                    "Error: ${_recognitionResult!['error']}",
                                    style: const TextStyle(color: Colors.red, fontSize: 14),
                                  ),
                                ),
                              ],
                            ),
                          ),
                        ],
                      ],
                    ),
                  ),
                ),
              ],
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildResultRow(String label, String value) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 3),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          SizedBox(
            width: 120,
            child: Text(
              "$label:",
              style: const TextStyle(fontWeight: FontWeight.w600, fontSize: 14),
            ),
          ),
          Expanded(
            child: Text(
              value.isNotEmpty ? value : "N/A",
              style: const TextStyle(fontSize: 14),
            ),
          ),
        ],
      ),
    );
  }

  bool _hasOcrFields() {
    return _recognitionResult!.keys.any((key) => key.startsWith('ocr_'));
  }

  bool _hasChipFields() {
    return _recognitionResult!.keys.any((key) => key.startsWith('chip_'));
  }

  List<Widget> _buildOcrFields() {
    List<Widget> widgets = [];

    if (_recognitionResult!.containsKey('ocr_fields')) {
      Map<String, dynamic> ocrFields = _recognitionResult!['ocr_fields'];
      for (String key in ocrFields.keys) {
        widgets.add(_buildResultRow(key.replaceAll('_', ' ').toUpperCase(), ocrFields[key].toString()));
      }
    } else {
      // Handle direct OCR fields (ocr_name, ocr_id_number, etc.)
      for (String key in _recognitionResult!.keys) {
        if (key.startsWith('ocr_')) {
          String displayName = key.substring(4).replaceAll('_', ' ').toUpperCase();
          widgets.add(_buildResultRow(displayName, _recognitionResult![key].toString()));
        }
      }
    }

    return widgets;
  }

  List<Widget> _buildChipFields() {
    List<Widget> widgets = [];

    if (_recognitionResult!.containsKey('chip_fields')) {
      Map<String, dynamic> chipFields = _recognitionResult!['chip_fields'];
      for (String key in chipFields.keys) {
        widgets.add(_buildResultRow(key.replaceAll('_', ' ').toUpperCase(), chipFields[key].toString()));
      }
    } else {
      // Handle direct chip fields (chip_name, chip_id_number, etc.)
      for (String key in _recognitionResult!.keys) {
        if (key.startsWith('chip_')) {
          String displayName = key.substring(5).replaceAll('_', ' ').toUpperCase();
          widgets.add(_buildResultRow(displayName, _recognitionResult![key].toString()));
        }
      }
    }

    return widgets;
  }

  String _getCardTypeDescription(String cardType) {
    try {
      int type = int.parse(cardType);
      List<String> descriptions = [];

      if (type & 1 != 0) descriptions.add("Has Chip");
      if (type & 2 != 0) descriptions.add("No Chip");
      if (type & 4 != 0) descriptions.add("Has Barcode");

      return descriptions.isNotEmpty ? descriptions.join(", ") : "Unknown ($cardType)";
    } catch (e) {
      return cardType;
    }
  }
}