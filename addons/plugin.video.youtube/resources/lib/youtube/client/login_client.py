__author__ = 'bromix'

import time
import urlparse

from resources.lib.kodion import simple_requests as requests
from resources.lib.youtube.youtube_exceptions import LoginException


class LoginClient(object):
    CONFIGS = {
        'youtube-tv': {
            'system': 'All',
            'key': 'AIzaSyAd-YEOqZz9nXVzGtn3KWzYLbLaajhqIDA',
            'id': '861556708454-d6dlm3lh05idd8npek18k6be8ba3oc68.apps.googleusercontent.com',
            'secret': 'SboVhoG9s0rNafixCSGGKXAT'
        },
        # API KEY for search and channel infos. These should work most of the time without login to safe some quota
        'youtube-for-kodi-quota': {
            'token-allowed': False,
            'system': 'All',
            'key': 'AIzaSyA7v1QOHz8Q4my5J8uGSpr0zRrntRjnMmk',
            'id': '597640352045-7um2gr1v5rgobm5bf07ebesm3er48286.apps.googleusercontent.com',
            'secret': 'VmyQ12KkJ_N3yegu4Y-VGCXd'
        },
        'youtube-for-kodi-fallback': {
            'token-allowed': False,
            'system': 'Fallback!',
            'key': 'AIzaSyBEvxICg_E5g8mfndYjWHH4WEDF2fN_zXM',
            'id': '705000440035-f7v8mhn75udt6l20r75ko7f15c39ns1c.apps.googleusercontent.com',
            'secret': 'XeLKD8o_mdzEBDy9Nw5KMKbr'
        },
        'youtube-for-kodi-12': {
            'system': 'Frodo',
            'key': 'AIzaSyB6-pMlWO_XmgdM15VKGeLH4QsipdToBas',
            'id': '131835494776-s0ef9jorci9vl0kaa5sqslupqvlku6ej.apps.googleusercontent.com',
            'secret': 'Fz9nnfVGoH6jiLc0iefvzZYM'
        },
        'youtube-for-kodi-13': {
            'system': 'Gotham',
            'key': 'AIzaSyB6-pMlWO_XmgdM15VKGeLH4QsipdToBas',
            'id': '131835494776-s0ef9jorci9vl0kaa5sqslupqvlku6ej.apps.googleusercontent.com',
            'secret': 'Fz9nnfVGoH6jiLc0iefvzZYM'
        },
        'youtube-for-kodi-14': {
            'system': 'Helix',
            'key': 'AIzaSyCOxb_JvsI1-ZneJ2SXTkwZvW3fdtaR3bg',
            'id': '45906745725-ucn38a9eumqcgfkmv14th17s0eq8kt5f.apps.googleusercontent.com',
            'secret': 'sK6GK-ZhzgXWehCdjaGybHRi'
        },
        'youtube-for-kodi-15': {
            'system': 'Isengard',
            'key': 'AIzaSyBAdxZCHbeJwnQ7dDZQJNfcaF46MdqJ24E',
            'id': '988476436049-l9sfcivj4jtm1f2ppcvvbgsli6kvtis0.apps.googleusercontent.com',
            'secret': '_BEENv-a3-egDz_QKo5pGZCK'
        },
        'youtube-for-kodi-16': {
            'system': 'Jarvis',
            'key': 'AIzaSyAb27K8eMR9ysX2-sdE7TqVADV1wpJC0Es',
            'id': '348423189006-rpgeltdbe7tf9rtj7rs2c14md67cqgd3.apps.googleusercontent.com',
            'secret': 'jbJEvwaKrjucp_JJjE1iPLV3'
        }
    }

    def __init__(self, config={}, language='en-US', access_token='', access_token_tv=''):
        if not config:
            config = self.CONFIGS['youtube-for-kodi-fallback']
            pass

        self._config = config
        self._config_tv = self.CONFIGS['youtube-tv']

        # the default language is always en_US (like YouTube on the WEB)
        if not language:
            language = 'en_US'
            pass

        language = language.replace('-', '_')
        language_components = language.split('_')
        if len(language_components) != 2:
            language = 'en_US'
            pass

        self._language = language
        self._country = language.split('_')[1]
        self._access_token = access_token
        self._access_token_tv = access_token_tv
        self._log_error_callback = None
        pass

    def set_log_error(self, callback):
        self._log_error_callback = callback
        pass

    def log_error(self, text):
        if self._log_error_callback:
            self._log_error_callback(text)
            pass
        else:
            print text
            pass
        pass

    def revoke(self, refresh_token):
        headers = {'Host': 'www.youtube.com',
                   'Connection': 'keep-alive',
                   'Origin': 'https://www.youtube.com',
                   'User-Agent': 'Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/40.0.2214.28 Safari/537.36',
                   'Content-Type': 'application/x-www-form-urlencoded;charset=UTF-8',
                   'Accept': '*/*',
                   'DNT': '1',
                   'Referer': 'https://www.youtube.com/tv',
                   'Accept-Encoding': 'gzip, deflate',
                   'Accept-Language': 'en-US,en;q=0.8,de;q=0.6'}

        post_data = {'token': refresh_token}

        # url
        url = 'https://www.youtube.com/o/oauth2/revoke'

        result = requests.post(url, data=post_data, headers=headers, verify=False)
        if result.status_code != requests.codes.ok:
            raise LoginException('Logout Failed')

        pass

    def refresh_token_tv(self, refresh_token, grant_type=''):
        client_id = self.CONFIGS['youtube-tv']['id']
        client_secret = self.CONFIGS['youtube-tv']['secret']
        return self.refresh_token(refresh_token, client_id=client_id, client_secret=client_secret,
                                  grant_type=grant_type)

    def refresh_token(self, refresh_token, client_id='', client_secret='', grant_type=''):
        headers = {'Host': 'www.youtube.com',
                   'Connection': 'keep-alive',
                   'Origin': 'https://www.youtube.com',
                   'User-Agent': 'Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/40.0.2214.28 Safari/537.36',
                   'Content-Type': 'application/x-www-form-urlencoded;charset=UTF-8',
                   'Accept': '*/*',
                   'DNT': '1',
                   'Referer': 'https://www.youtube.com/tv',
                   'Accept-Encoding': 'gzip, deflate',
                   'Accept-Language': 'en-US,en;q=0.8,de;q=0.6'}

        _client_id = client_id
        if not client_id:
            _client_id = self._config['id']
            pass
        _client_secret = client_secret
        if not _client_secret:
            _client_secret = self._config['secret']
            pass
        post_data = {'client_id': _client_id,
                     'client_secret': _client_secret,
                     'refresh_token': refresh_token,
                     'grant_type': 'refresh_token'}

        # url
        url = 'https://www.youtube.com/o/oauth2/token'

        result = requests.post(url, data=post_data, headers=headers, verify=False)
        if result.status_code != requests.codes.ok:
            raise LoginException('Login Failed')

        if result.headers.get('content-type', '').startswith('application/json'):
            json_data = result.json()
            access_token = json_data['access_token']
            expires_in = time.time() + int(json_data.get('expires_in', 3600))
            return access_token, expires_in

        return '', ''

    def get_device_token_tv(self, code, client_id='', client_secret='', grant_type=''):
        client_id = self.CONFIGS['youtube-tv']['id']
        client_secret = self.CONFIGS['youtube-tv']['secret']
        return self.get_device_token(code, client_id=client_id, client_secret=client_secret, grant_type=grant_type)

    def get_device_token(self, code, client_id='', client_secret='', grant_type=''):
        headers = {'Host': 'www.youtube.com',
                   'Connection': 'keep-alive',
                   'Origin': 'https://www.youtube.com',
                   'User-Agent': 'Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/40.0.2214.28 Safari/537.36',
                   'Content-Type': 'application/x-www-form-urlencoded;charset=UTF-8',
                   'Accept': '*/*',
                   'DNT': '1',
                   'Referer': 'https://www.youtube.com/tv',
                   'Accept-Encoding': 'gzip, deflate',
                   'Accept-Language': 'en-US,en;q=0.8,de;q=0.6'}

        _client_id = client_id
        if not client_id:
            _client_id = self._config['id']
            pass
        _client_secret = client_secret
        if not _client_secret:
            _client_secret = self._config['secret']
            pass
        post_data = {'client_id': _client_id,
                     'client_secret': _client_secret,
                     'code': code,
                     'grant_type': 'http://oauth.net/grant_type/device/1.0'}

        # url
        url = 'https://www.youtube.com/o/oauth2/token'

        result = requests.post(url, data=post_data, headers=headers, verify=False)
        if result.status_code != requests.codes.ok:
            raise LoginException('Login Failed')

        if result.headers.get('content-type', '').startswith('application/json'):
            return result.json()

        return None

    def generate_user_code_tv(self):
        client_id = self.CONFIGS['youtube-tv']['id']
        return self.generate_user_code(client_id=client_id)

    def generate_user_code(self, client_id=''):
        headers = {'Host': 'www.youtube.com',
                   'Connection': 'keep-alive',
                   'Origin': 'https://www.youtube.com',
                   'User-Agent': 'Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/40.0.2214.28 Safari/537.36',
                   'Content-Type': 'application/x-www-form-urlencoded;charset=UTF-8',
                   'Accept': '*/*',
                   'DNT': '1',
                   'Referer': 'https://www.youtube.com/tv',
                   'Accept-Encoding': 'gzip, deflate',
                   'Accept-Language': 'en-US,en;q=0.8,de;q=0.6'}

        _client_id = client_id
        if not client_id:
            _client_id = self._config['id']
        post_data = {'client_id': _client_id,
                     'scope': 'https://www.googleapis.com/auth/youtube'}
        # 'scope': 'http://gdata.youtube.com https://www.googleapis.com/auth/youtube-paid-content'}

        # url
        url = 'https://www.youtube.com/o/oauth2/device/code'

        result = requests.post(url, data=post_data, headers=headers, verify=False)
        if result.status_code != requests.codes.ok:
            raise LoginException('Login Failed')

        if result.headers.get('content-type', '').startswith('application/json'):
            return result.json()

        return None

    def get_access_token(self):
        return self._access_token

    def authenticate(self, username, password):
        headers = {'device': '38c6ee9a82b8b10a',
                   'app': 'com.google.android.youtube',
                   'User-Agent': 'GoogleAuth/1.4 (GT-I9100 KTU84Q)',
                   'content-type': 'application/x-www-form-urlencoded',
                   'Host': 'android.clients.google.com',
                   'Connection': 'Keep-Alive',
                   'Accept-Encoding': 'gzip'}

        post_data = {'device_country': self._country.lower(),
                     'operatorCountry': self._country.lower(),
                     'lang': self._language.replace('-', '_'),
                     'sdk_version': '19',
                     # 'google_play_services_version': '6188034',
                     'accountType': 'HOSTED_OR_GOOGLE',
                     'Email': username.encode('utf-8'),
                     'service': 'oauth2:https://www.googleapis.com/auth/youtube https://www.googleapis.com/auth/youtube.force-ssl https://www.googleapis.com/auth/plus.me https://www.googleapis.com/auth/emeraldsea.mobileapps.doritos.cookie https://www.googleapis.com/auth/plus.stream.read https://www.googleapis.com/auth/plus.stream.write https://www.googleapis.com/auth/plus.pages.manage https://www.googleapis.com/auth/identity.plus.page.impersonation',
                     'source': 'android',
                     'androidId': '38c6ee9a82b8b10a',
                     'app': 'com.google.android.youtube',
                     # 'client_sig': '24bb24c05e47e0aefa68a58a766179d9b613a600',
                     'callerPkg': 'com.google.android.youtube',
                     # 'callerSig': '24bb24c05e47e0aefa68a58a766179d9b613a600',
                     'Passwd': password.encode('utf-8')}

        # url
        url = 'https://android.clients.google.com/auth'

        result = requests.post(url, data=post_data, headers=headers, verify=False)
        if result.status_code != requests.codes.ok:
            raise LoginException('Login Failed')

        lines = result.text.replace('\n', '&')
        params = dict(urlparse.parse_qsl(lines))
        token = params.get('Auth', '')
        expires = int(params.get('Expiry', -1))
        if not token or expires == -1:
            raise LoginException('Failed to get token')

        return token, expires

    pass
