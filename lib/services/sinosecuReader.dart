import 'package:flutter/services.dart';

class SinosecuReader {
  static const MethodChannel _channel = MethodChannel('com.example.sino_scanner');

  static Future<int> initializeScanner({
    required String userId,
    required int nType,
    required String sdkDirectory,
  }) async {
    try {
      print('[Flutter] Calling initializeScanner with:');
      print('[Flutter]   UserID: $userId');
      print('[Flutter]   nType: $nType');
      print('[Flutter]   SDK Directory: $sdkDirectory');

      final int result = await _channel.invokeMethod('initializeScanner', {
        'userId': userId,
        'nType': nType,
        'sdkDirectory': sdkDirectory,
      });
      print('[Flutter] Scanner initialized. Native Result: $result');
      return result;
    } on PlatformException catch (e) {
      print('[Flutter] Failed to initialize scanner: ${e.message}');
      print('[Flutter] PlatformException Details: ${e.details}');
      return -100;
    } catch (e) {
      print('[Flutter] Unknown error during initializeScanner: $e');
      return -200;
    }
  }

  // The SDK's DetectDocument returns an int.
  static Future<int> detectDocument() async {
    try {
      final int result = await _channel.invokeMethod('detectDocument');
      print('[Flutter] detectDocument call. Native Result: $result');
      // Interpret result based on SDK documentation [cite: 36]
      // -1: Core engine not initialized
      //  0: Cannot detect
      //  1: Document placed
      //  2: Document taken out
      //  3: Detected mobile phone barcode (AR/KR series) [cite: 36]
      return result;
    } on PlatformException catch (e) {
      print('[Flutter] Failed to call detectDocument: ${e.message}');
      return -100;
    } catch (e) {
      print('[Flutter] Unknown error during detectDocument: $e');
      return -200;
    }
  }

  // The SDK's AutoProcessIDCard returns an int (main document type or error code) [cite: 38]
  // and has an output parameter nCardType. For platform channels, it's often easier
  // to return a Map from C++ containing both values.
  static Future<Map<String, dynamic>> autoProcessDocument() async {
    try {
      // If your C++ side returns a Map for autoProcess, this will work.
      // Example C++ return: {"mainType": result, "cardTypeDetails": nCardTypeValue}
      final Map<dynamic, dynamic>? result = await _channel.invokeMethod('autoProcessDocument');
      print('[Flutter] autoProcessDocument call. Native Result: $result');
      if (result != null) {
        return Map<String, dynamic>.from(result);
      }
      return {"error": "Null result from native", "mainType": -999, "cardTypeDetails": -999};
    } on PlatformException catch (e) {
      print('[Flutter] Failed to call autoProcessDocument: ${e.message}');
      return {"error": e.message, "mainType": -100, "cardTypeDetails": -100};
    } catch (e) {
      print('[Flutter] Unknown error during autoProcessDocument: $e');
      return {"error": e.toString(), "mainType": -200, "cardTypeDetails": -200};
    }
  }

  static Future<void> releaseScanner() async {
    try {
      await _channel.invokeMethod('releaseScanner');
      print('[Flutter] Scanner release method called.');
    } on PlatformException catch (e) {
      print('[Flutter] Failed to release scanner: ${e.message}');
    } catch (e) {
      print('[Flutter] Unknown error during releaseScanner: $e');
    }
  }
}