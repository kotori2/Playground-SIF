/* 
   Copyright 2013 KLab Inc.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#include "assert.h"
#include <stdio.h>
#include <stdarg.h>

#include "CPFInterface.h"

#include "CiOSPlatform.h"

extern "C" {
void assertFunction(int line, const char* file, const char* msg,...) {
	va_list	argp;
	char pszBuf [1024];

	va_start(argp, msg);
	vsprintf( pszBuf, msg, argp);
	va_end(argp);

    char alertBuf[4096];
    sprintf(alertBuf, "Assert l.%i in %s : %s", line, file, pszBuf);
    NSString * alert_msg = [NSString stringWithCString:alertBuf encoding:NSUTF8StringEncoding];

	NSLog(@"%s", alertBuf); 
    
    CiOSPlatform * pPlatform = CiOSPlatform::getInstance();
    UIAlertController *alertController = [UIAlertController alertControllerWithTitle:@"Assertion"
                                                                             message:alert_msg
                                                                      preferredStyle:UIAlertControllerStyleAlert];

    UIAlertAction *abortButton = [UIAlertAction actionWithTitle:@"abort"
                                                    style:UIAlertActionStyleDefault
                                                  handler:^(UIAlertAction *action) {
                                                    abort();
                                                  }];

    [alertController addAction:abortButton];

    [pPlatform->m_pViewController presentViewController:alertController animated:YES completion:nil];
    
    // TODO: crash the game once alert generated.
}
    
void msgBox(char * log) {

}
}
