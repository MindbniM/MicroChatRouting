import os
import sys

from typing import List

from alibabacloud_dysmsapi20170525.client import Client as Dysmsapi20170525Client
from alibabacloud_tea_openapi import models as open_api_models
from alibabacloud_dysmsapi20170525 import models as dysmsapi_20170525_models
from alibabacloud_tea_util import models as util_models
from alibabacloud_tea_util.client import Client as UtilClient
class DMSClient:
    def __init__(self):
        config = open_api_models.Config(
            access_key_id=os.environ.get('ALIBABA_CLOUD_ACCESS_KEY_ID'),
            access_key_secret=os.environ.get('ALIBABA_CLOUD_ACCESS_KEY_SECRET')
        )
        config.endpoint = f'dysmsapi.aliyuncs.com'
        self._client=Dysmsapi20170525Client(config)


    def send(self,phone,code):
        send_sms_request = dysmsapi_20170525_models.SendSmsRequest(
            sign_name='M消息路由',
            template_code='SMS_475110088',
            phone_numbers=phone,
            template_param=f'{{"code":"{code}"}}'
        )
        runtime = util_models.RuntimeOptions()
        try:
            response=self._client.send_sms_with_options(send_sms_request, runtime)
            return ""
        except Exception as error:
            return error.message
