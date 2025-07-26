import requests
import json
import os
import hashlib

BLARGG_ROM_BASE = "https://github.com/retrio/gb-test-roms/raw/refs/heads/master"
GB_DOCTOR_LOG_BASE = "https://github.com/robert/gameboy-doctor/raw/refs/heads/master/truth/zipped/cpu_instrs"
TEST_DATA_DIR = "testdata"

class Dependency:
    def __init__(self, url: str, path: str, sha256: str):
        self.url = url
        self.path = path
        self.sha256 = sha256

        os.makedirs(os.path.dirname(self.path), exist_ok=True)

    def calculate_sha256(self):
        with open(self.path, "rb") as f:
            data = f.read()
        return hashlib.sha256(data).hexdigest()

    def check(self):
        if not os.path.exists(self.path):
            return False
        
        if self.sha256 is None:
            return False
        
        return self.calculate_sha256() == self.sha256

def blargg_cpu(rom_name: str, sha256: str):
    return Dependency(f"{BLARGG_ROM_BASE}/cpu_instrs/individual/{rom_name}", f"{TEST_DATA_DIR}/blargg/cpu_instrs/{rom_name}", sha256)

def gb_doctor(log_name: str, sha256: str):
    return Dependency(f"{GB_DOCTOR_LOG_BASE}/{log_name}", f"{TEST_DATA_DIR}/blargg/cpu_instrs/{log_name}", sha256)

def blargg_general(rom_name: str, git_path: str, sha256: str):
    return Dependency(f"{BLARGG_ROM_BASE}/{git_path}/{rom_name}", f"{TEST_DATA_DIR}/blargg/general/{rom_name}", sha256)

dependencies = [

    Dependency("https://github.com/SingleStepTests/sm83/archive/refs/heads/main.zip", f"{TEST_DATA_DIR}/sm83.zip", "87780d9d2eba95ac7836ef3d5b7d48a390a9d75eea0d3972ee6f9c3bf431463b"),

    blargg_cpu("01-special.gb", "fe61349cbaee10cc384b50f356e541c90d1bc380185716706b5d8c465a03cf89"),
    blargg_cpu("02-interrupts.gb", "fb90b0d2b9501910c49709abda1d8e70f757dc12020ebf8409a7779bbfd12229"),
    blargg_cpu("03-op sp,hl.gb", "ca553e606d9b9c86fbd318f1b916c6f0b9df0cf1774825d4361a3fdff2e5a136"),
    blargg_cpu("04-op r,imm.gb", "7686aa7a39ef3d2520ec1037371b5f94dc283fbbfd0f5051d1f64d987bdd6671"),
    blargg_cpu("05-op rp.gb", "d504adfa0a4c4793436a154f14492f044d38b3c6db9efc44138f3c9ad138b775"),
    blargg_cpu("06-ld r,r.gb", "17ada54b0b9c1a33cd5429fce5b765e42392189ca36da96312222ffe309e7ed1"),
    blargg_cpu("07-jr,jp,call,ret,rst.gb", "ab31d3daaaa3a98bdbd9395b64f48c1bdaa889aba5b19dd5aaff4ec2a7d228a3"),
    blargg_cpu("08-misc instrs.gb", "974a71fe4c67f70f5cc6e98d4dc8c096057ff8a028b7bfa9f7a4330038cf8b7e"),
    blargg_cpu("09-op r,r.gb", "b28e1be5cd95f22bd1ecacdd33c6f03e607d68870e31a47b15a0229033d5ba2a"),
    blargg_cpu("10-bit ops.gb", "7f5b8e488c6988b5aaba8c2a74529b7c180c55a58449d5ee89d606a07c53514a"),
    blargg_cpu("11-op a,(hl).gb", "0ec0cf9fda3f00becaefa476df6fb526c434abd9d4a4beac237c2c2692dac5d3"),

    gb_doctor("1.zip", "8e9f36db0a7196f222d7c526da4bcb73d38ce42ccef68ef1a73672db3875d32f"),
    gb_doctor("2.zip", "937ab150bb823688b4bbd1cd5740263e2855205bea5c6140dc1e5b0f87424bc5"),
    gb_doctor("3.zip", "6435e1360d32691dd77ed210b1ebe1ee2bf404911cc3d52df79c6a557563e13b"),
    gb_doctor("4.zip", "092370f82b62cc646488c4157377d1fb62851718380a25bd1f8ee1d522395d74"),
    gb_doctor("5.zip", "6e7027a872fb292535967156d16085b9b42d20ab28fd5bfc6eef870305f2389a"),
    gb_doctor("6.zip", "1642c2c15984b7cd1eade83b37cf0fe6ad334ff83f00c31ada2c323668a69116"),
    gb_doctor("7.zip", "6ea9fe9063ad0e0342c4ed129170be137924abad27fd0ade51b736c2552b2000"),
    gb_doctor("8.zip", "e093c6f13a37271c357886a15b572be5be8d04a86b610fabc8e75d5bdc8aae6c"),
    gb_doctor("9.zip", "4e4df62a097ebd348877fac4dd553c541b6c873b6a31d308a85affaab1a71592"),
    gb_doctor("10.zip", "d6dfd53e8df92cd07dd88f5b32e2c6b0fc4fd6777da9d2848c9aa1f010f97ac2"),
    gb_doctor("11.zip", "d1eb0b504da593b68cb1d9954fcc97ec15c7d7a42a92cc61e3198c7b2c447f5b"),

    blargg_general("instr_timing.gb", "instr_timing", "646067b3d6c79fda810e9c3f1cb7c0efd5abb0a7ac06437c54e65720c15d9925"),
    blargg_general("01-read_timing.gb", "mem_timing/individual", "52724532c5709e38e947eb429337c124c38bc68f373874435a7460548098b617"),
    blargg_general("02-write_timing.gb", "mem_timing/individual", "eea92d3f4e95aab5910e0f7080916a3c42a2b8deae1ee5d45d1e3751d648f3f6"),
    blargg_general("03-modify_timing.gb", "mem_timing/individual", "2e9067c670ff8b45916bf321677ad04a6896d06a057dbcb82ae9f208a1ae9c34"),


]

for dependency in dependencies:
    if not dependency.check():
        print(f"Downloading {dependency.url} to {dependency.path}")
        response = requests.get(dependency.url, stream=True)

        if response.status_code < 200 or response.status_code >= 300:
            print(f"Failed to download {dependency.url}, status code: {response.status_code}")
            exit(1)

        with open(dependency.path, "wb") as f:
            for chunk in response.iter_content(chunk_size=8192):
                f.write(chunk)

        result = dependency.check()
        if result is False:
            print(f"SHA256 mismatch for {dependency.path}, got {dependency.calculate_sha256()}, expected {dependency.sha256}")
            os.remove(dependency.path)
            exit(1)

    else:
        print(f"Skipping {dependency.path} because it already exists")

print("Done")