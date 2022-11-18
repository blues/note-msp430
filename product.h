
// This is the unique Product Identifier for your device.  This Product ID tells the Notecard what
// type of device has embedded the Notecard, and by extension which vendor or customer is in charge
// of "managing" it.  In order to set this value, you must first register with notehub.io and
// "claim" a unique product ID for your device.  It could be something as simple as as your email
// address in reverse, such as "com.gmail.smith.lisa.test-device" or "com.outlook.gates.bill.demo"

#ifndef PRODUCT_UID
#pragma message "PRODUCT_UID is not defined in this example. Please ensure your Notecard has a product identifier set before running this example or define it in code here. More details at https://bit.ly/product-uid"
#endif
// #define PRODUCT_UID "com.my-company.my-name:my-project"