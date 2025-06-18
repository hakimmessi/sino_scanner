class PassportData {
  final String? passportNumber;
  final String? englishName;
  final String? gender;
  final String? dateOfBirth;
  final String? dateOfExpiry;
  final String? issuingCountryCode;
  final String? englishSurname;
  final String? englishFirstName;
  final String? mrzLine1;
  final String? mrzLine2;
  final String? nationalityCode;
  final String? placeOfBirth;
  final String? placeOfIssue;
  final String? dateOfIssue;
  final Map<String, dynamic> rawData;

  PassportData({
    this.passportNumber,
    this.englishName,
    this.gender,
    this.dateOfBirth,
    this.dateOfExpiry,
    this.issuingCountryCode,
    this.englishSurname,
    this.englishFirstName,
    this.mrzLine1,
    this.mrzLine2,
    this.nationalityCode,
    this.placeOfBirth,
    this.placeOfIssue,
    this.dateOfIssue,
    required this.rawData,
  });

  factory PassportData.fromMap(Map<String, dynamic> data, {bool validateData = false}) {
    // Handle both OCR prefixed and direct field names
    String? getField(String fieldName) {
      return data['ocr_$fieldName'] ?? data[fieldName];
    }

    String? passportNum = getField('passport_number_mrz') ?? getField('passport_number');
    String? name = getField('english_name') ?? getField('name');
    String? sex = getField('gender') ?? getField('sex');
    String? birthDate = getField('date_of_birth');
    String? expiryDate = getField('date_of_expiry') ?? getField('expiry_date');
    String? countryCode = getField('issuing_country_code');
    String? surname = getField('english_surname') ?? getField('Postname');
    String? firstName = getField('english_first_name') ?? getField('Given_names');
    String? mrz1 = getField('mrz_line_1') ?? getField('mrz1');
    String? mrz2 = getField('mrz_line_2') ?? getField('mrz2');
    String? nationality = getField('nationality_code') ?? getField('nationality');
    String? birthPlace = getField('place_of_birth');
    String? issuePlace = getField('place_of_issue') ?? getField('Issuing_country');
    String? issueDate = getField('date_of_issue');

    return PassportData(
      passportNumber: passportNum?.trim(),
      englishName: name?.trim(),
      gender: sex?.trim(),
      dateOfBirth: birthDate?.trim(),
      dateOfExpiry: expiryDate?.trim(),
      issuingCountryCode: countryCode?.trim(),
      englishSurname: surname?.trim(),
      englishFirstName: firstName?.trim(),
      mrzLine1: mrz1?.trim(),
      mrzLine2: mrz2?.trim(),
      nationalityCode: nationality?.trim(),
      placeOfBirth: birthPlace?.trim(),
      placeOfIssue: issuePlace?.trim(),
      dateOfIssue: issueDate?.trim(),
      rawData: data,
    );
  }

  Map<String, dynamic> toMap() {
    return {
      'passportNumber': passportNumber,
      'englishName': englishName,
      'gender': gender,
      'dateOfBirth': dateOfBirth,
      'dateOfExpiry': dateOfExpiry,
      'issuingCountryCode': issuingCountryCode,
      'englishSurname': englishSurname,
      'englishFirstName': englishFirstName,
      'mrzLine1': mrzLine1,
      'mrzLine2': mrzLine2,
      'nationalityCode': nationalityCode,
      'placeOfBirth': placeOfBirth,
      'placeOfIssue': placeOfIssue,
      'dateOfIssue': dateOfIssue,
    };
  }

  bool get isValid {
    return passportNumber != null &&
        englishName != null &&
        gender != null &&
        dateOfBirth != null;
  }

  @override
  String toString() {
    return 'PassportData{passportNumber: $passportNumber, englishName: $englishName, gender: $gender}';
  }
}