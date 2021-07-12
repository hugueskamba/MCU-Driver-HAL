/* Copyright (c) 2020-2021 Arm Limited
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "hal/pwmout_api.h"

int main(void)
{
   pwmout_t my_pwm1;
   pwmout_t my_pwm2;

   pwmout_init(&my_pwm1, LED1);
   pwmout_init(&my_pwm2, LED2);

   pwmout_period_ms(&my_pwm1, 1);
   pwmout_period_ms(&my_pwm2, 1);

   while (1) {

     for (int j = 0; j < 100; j++){
       pwmout_write(&my_pwm1, 1-j/100.0);
       pwmout_write(&my_pwm2, j/100.0);
       for (unsigned long i = 0; i < 200000UL; i++);
     }
     for (int j = 0; j < 100; j++){
       pwmout_write(&my_pwm1, j/100.0);
       pwmout_write(&my_pwm2, 1-j/100.0);
       for (unsigned long i = 0; i < 200000UL; i++);
     }

   }
}
