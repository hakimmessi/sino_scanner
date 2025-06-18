import 'package:flutter/material.dart';
import 'package:flutter_svg/flutter_svg.dart';
import 'package:sino_scanner/services/PassportData.dart';
import 'package:sino_scanner/services/ScanResult.dart';
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
  PassportData? _passportData;
  Map<String, dynamic>? _rawScanData;
  List<String>? _savedImagePaths;
  bool _saveImages = true;
  bool _enableDebugMode = false;

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
      _passportData = null;
      _rawScanData = null;
      _savedImagePaths = null;
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
      _passportData = null;
      _rawScanData = null;
      _savedImagePaths = null;
    });

    try {
      // Use the enhanced scan method
      ScanResult result = await SinosecuReader.performCompleteScan(
        timeoutSeconds: 20,
        enableDebug: _enableDebugMode,
        validateData: true,
        saveImages: _saveImages,
        customName: 'passport_${DateTime.now().millisecondsSinceEpoch}',
        cleanupOld: true,
      );

      if (result.success) {
        setState(() {
          _statusText = 'Document scanned successfully!';
          _passportData = result.passportData;
          _rawScanData = result.rawScanData;
          _savedImagePaths = result.savedImagePaths;
          _isScanning = false;
        });
      } else {
        setState(() {
          _statusText = 'Scan failed: ${result.error}';
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

  Future<void> _debugFields() async {
    if (!_isScannerInitialized) {
      setState(() {
        _statusText = "Please initialize scanner first";
      });
      return;
    }

    setState(() {
      _statusText = 'Starting debug field scan...\nCheck console for details.';
    });

    try {
      await SinosecuReader.debugAllAvailableFields(1); // OCR fields
      await SinosecuReader.debugAllAvailableFields(0); // Chip fields

      setState(() {
        _statusText = 'Debug scan completed. Check console logs.';
      });
    } catch (e) {
      setState(() {
        _statusText = 'Debug error: $e';
      });
    }
  }

  Future<void> _viewSavedImages() async {
    try {
      List<String> imagePaths = await SinosecuReader.getAllPassportImages();
      if (imagePaths.isEmpty) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('No saved images found')),
        );
        return;
      }

      showDialog(
        context: context,
        builder: (context) => AlertDialog(
          title: const Text('Saved Images'),
          content: SizedBox(
            width: double.maxFinite,
            height: 300,
            child: ListView.builder(
              itemCount: imagePaths.length,
              itemBuilder: (context, index) {
                String imagePath = imagePaths[index];
                String fileName = imagePath.split('/').last;

                return ListTile(
                  leading: const Icon(Icons.image),
                  title: Text(fileName),
                  subtitle: Text(imagePath),
                  onTap: () {
                    // You could implement image viewing here
                    Navigator.of(context).pop();
                    ScaffoldMessenger.of(context).showSnackBar(
                      SnackBar(content: Text('Image path: $imagePath')),
                    );
                  },
                );
              },
            ),
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.of(context).pop(),
              child: const Text('Close'),
            ),
          ],
        ),
      );
    } catch (e) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Error loading images: $e')),
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.white,
      appBar: AppBar(
        title: const Text('Sinosecu Document Scanner'),
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
        actions: [
          if (_isScannerInitialized) ...[
            IconButton(
              icon: const Icon(Icons.settings),
              onPressed: () => _showSettingsDialog(),
            ),
            IconButton(
              icon: const Icon(Icons.image),
              onPressed: _viewSavedImages,
            ),
          ],
        ],
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
                    foregroundColor: Colors.black87,
                    padding: const EdgeInsets.symmetric(horizontal: 40, vertical: 20),
                    shape: RoundedRectangleBorder(
                      borderRadius: BorderRadius.circular(30),
                      side: const BorderSide(color: Colors.black87, width: 2),
                    ),
                    elevation: 0,
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
                              side: const BorderSide(color: Colors.black87, width: 2),
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
                    Expanded(
                      child: Container(
                        margin: const EdgeInsets.only(left: 8),
                        child: ElevatedButton(
                          onPressed: _isScanning ? null : _debugFields,
                          style: ElevatedButton.styleFrom(
                            backgroundColor: Colors.orange,
                            foregroundColor: Colors.white,
                            padding: const EdgeInsets.symmetric(horizontal: 40, vertical: 20),
                            shape: RoundedRectangleBorder(
                              borderRadius: BorderRadius.circular(30),
                            ),
                            elevation: 0,
                          ),
                          child: const Text('Debug Fields', style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold)),
                        ),
                      ),
                    ),
                  ],
                ),
                const SizedBox(height: 15),

                // Settings toggles
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                  children: [
                    Row(
                      children: [
                        Checkbox(
                          value: _saveImages,
                          onChanged: (value) => setState(() => _saveImages = value ?? true),
                        ),
                        const Text('Save Images'),
                      ],
                    ),
                    Row(
                      children: [
                        Checkbox(
                          value: _enableDebugMode,
                          onChanged: (value) => setState(() => _enableDebugMode = value ?? false),
                        ),
                        const Text('Debug Mode'),
                      ],
                    ),
                  ],
                ),

                // Reset button
                ElevatedButton(
                  onPressed: () {
                    setState(() {
                      _statusText = "Scanner ready";
                      _passportData = null;
                      _rawScanData = null;
                      _savedImagePaths = null;
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

              // Passport Data Display (Enhanced)
              if (_passportData != null) ...[
                _buildPassportDataCard(),
              ],

              // Raw Data Display (Collapsible)
              if (_rawScanData != null) ...[
                const SizedBox(height: 10),
                _buildRawDataCard(),
              ],

              // Saved Images Display
              if (_savedImagePaths != null && _savedImagePaths!.isNotEmpty) ...[
                const SizedBox(height: 10),
                _buildSavedImagesCard(),
              ],
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildPassportDataCard() {
    return Card(
      elevation: 4,
      margin: const EdgeInsets.symmetric(vertical: 10),
      child: ExpansionTile(
        title: Row(
          children: [
            Icon(Icons.check_circle, color: Colors.green, size: 24),
            const SizedBox(width: 8),
            const Text(
              "Passport Information",
              style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold, color: Colors.green),
            ),
          ],
        ),
        initiallyExpanded: true,
        children: [
          Container(
            width: double.infinity,
            padding: const EdgeInsets.all(20.0),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                if (_passportData!.passportNumber != null)
                  _buildResultRow("Passport Number", _passportData!.passportNumber!),
                if (_passportData!.englishName != null)
                  _buildResultRow("Full Name", _passportData!.englishName!),
                if (_passportData!.englishSurname != null)
                  _buildResultRow("Surname", _passportData!.englishSurname!),
                if (_passportData!.englishFirstName != null)
                  _buildResultRow("First Name(s)", _passportData!.englishFirstName!),
                if (_passportData!.gender != null)
                  _buildResultRow("Gender", _passportData!.gender!),
                if (_passportData!.dateOfBirth != null)
                  _buildResultRow("Date of Birth", _formatDate(_passportData!.dateOfBirth!)),
                if (_passportData!.dateOfExpiry != null)
                  _buildResultRow("Date of Expiry", _formatDate(_passportData!.dateOfExpiry!)),
                if (_passportData!.issuingCountryCode != null)
                  _buildResultRow("Issuing Country", _passportData!.issuingCountryCode!),
                if (_passportData!.nationalityCode != null)
                  _buildResultRow("Nationality", _passportData!.nationalityCode!),
                if (_passportData!.placeOfBirth != null)
                  _buildResultRow("Place of Birth", _passportData!.placeOfBirth!),
                if (_passportData!.placeOfIssue != null)
                  _buildResultRow("Place of Issue", _passportData!.placeOfIssue!),
                if (_passportData!.dateOfIssue != null)
                  _buildResultRow("Date of Issue", _formatDate(_passportData!.dateOfIssue!)),

                // MRZ Data
                if (_passportData!.mrzLine1 != null || _passportData!.mrzLine2 != null) ...[
                  const SizedBox(height: 15),
                  const Text("Machine Readable Zone (MRZ):",
                      style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold)),
                  const SizedBox(height: 5),
                  if (_passportData!.mrzLine1 != null)
                    Container(
                      padding: const EdgeInsets.all(8),
                      decoration: BoxDecoration(
                        color: Colors.grey[100],
                        borderRadius: BorderRadius.circular(4),
                        border: Border.all(color: Colors.grey[300]!),
                      ),
                      child: Text(
                        _passportData!.mrzLine1!,
                        style: const TextStyle(fontFamily: 'monospace', fontSize: 12),
                      ),
                    ),
                  if (_passportData!.mrzLine2 != null) ...[
                    const SizedBox(height: 4),
                    Container(
                      padding: const EdgeInsets.all(8),
                      decoration: BoxDecoration(
                        color: Colors.grey[100],
                        borderRadius: BorderRadius.circular(4),
                        border: Border.all(color: Colors.grey[300]!),
                      ),
                      child: Text(
                        _passportData!.mrzLine2!,
                        style: const TextStyle(fontFamily: 'monospace', fontSize: 12),
                      ),
                    ),
                  ],
                ],

                // Validation Status
                const SizedBox(height: 15),
                Container(
                  padding: const EdgeInsets.all(10),
                  decoration: BoxDecoration(
                    color: _passportData!.isValid ? Colors.green[50] : Colors.orange[50],
                    borderRadius: BorderRadius.circular(8),
                    border: Border.all(
                        color: _passportData!.isValid ? Colors.green[200]! : Colors.orange[200]!
                    ),
                  ),
                  child: Row(
                    children: [
                      Icon(
                        _passportData!.isValid ? Icons.verified : Icons.warning,
                        color: _passportData!.isValid ? Colors.green : Colors.orange,
                        size: 20,
                      ),
                      const SizedBox(width: 8),
                      Text(
                        _passportData!.isValid ? "Valid passport data" : "Incomplete passport data",
                        style: TextStyle(
                          color: _passportData!.isValid ? Colors.green[700] : Colors.orange[700],
                          fontSize: 14,
                          fontWeight: FontWeight.w600,
                        ),
                      ),
                    ],
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildRawDataCard() {
    return Card(
      elevation: 2,
      margin: const EdgeInsets.symmetric(vertical: 5),
      child: ExpansionTile(
        title: const Text("Raw Scan Data", style: TextStyle(fontSize: 16, fontWeight: FontWeight.w600)),
        children: [
          Container(
            width: double.infinity,
            padding: const EdgeInsets.all(20.0),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                for (String key in _rawScanData!.keys)
                  if (!key.startsWith('ocr_') && !key.startsWith('chip_'))
                    _buildResultRow(key.replaceAll('_', ' ').toUpperCase(),
                        _rawScanData![key].toString()),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildSavedImagesCard() {
    return Card(
      elevation: 2,
      margin: const EdgeInsets.symmetric(vertical: 5),
      child: ExpansionTile(
        title: Text("Saved Images (${_savedImagePaths!.length})",
            style: const TextStyle(fontSize: 16, fontWeight: FontWeight.w600)),
        children: [
          Container(
            width: double.infinity,
            padding: const EdgeInsets.all(20.0),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                for (String imagePath in _savedImagePaths!)
                  Padding(
                    padding: const EdgeInsets.symmetric(vertical: 2),
                    child: Row(
                      children: [
                        const Icon(Icons.image, size: 16, color: Colors.blue),
                        const SizedBox(width: 8),
                        Expanded(
                          child: Text(
                            imagePath.split('/').last,
                            style: const TextStyle(fontSize: 12),
                          ),
                        ),
                      ],
                    ),
                  ),
              ],
            ),
          ),
        ],
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

  String _formatDate(String dateString) {
    if (dateString.length == 8) {
      try {
        String year = dateString.substring(0, 4);
        String month = dateString.substring(4, 6);
        String day = dateString.substring(6, 8);
        return "$day/$month/$year";
      } catch (e) {
        return dateString;
      }
    }
    return dateString;
  }

  void _showSettingsDialog() {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Scanner Settings'),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            CheckboxListTile(
              title: const Text('Save Images'),
              subtitle: const Text('Automatically save passport images'),
              value: _saveImages,
              onChanged: (value) {
                setState(() => _saveImages = value ?? true);
                Navigator.of(context).pop();
              },
            ),
            CheckboxListTile(
              title: const Text('Debug Mode'),
              subtitle: const Text('Enable detailed logging'),
              value: _enableDebugMode,
              onChanged: (value) {
                setState(() => _enableDebugMode = value ?? false);
                Navigator.of(context).pop();
              },
            ),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.of(context).pop(),
            child: const Text('Close'),
          ),
        ],
      ),
    );
  }
}