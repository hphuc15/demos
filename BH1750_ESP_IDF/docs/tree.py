from pathlib import Path

def print_tree(dir_path: Path, prefix: str = ""):
    # Lấy danh sách tất cả file và thư mục con, sắp xếp thư mục lên trước
    paths = sorted(list(dir_path.iterdir()), key=lambda p: (p.is_file(), p.name.lower()))
    
    for i, path in enumerate(paths):
        # Kiểm tra xem đây có phải là phần tử cuối cùng trong thư mục hiện tại không
        is_last = (i == len(paths) - 1)
        
        # Chọn ký tự nhánh phù hợp
        connector = "└── " if is_last else "├── "
        
        # In ra tên file/thư mục kèm nhánh
        print(f"{prefix}{connector}{path.name}")
        
        # Nếu là thư mục, tiếp tục đệ quy vào bên trong
        if path.is_dir():
            # Nếu là phần tử cuối, nhánh sau sẽ trống, ngược lại sẽ có đường kẻ đứng "|"
            new_prefix = prefix + ("    " if is_last else "│   ")
            print_tree(path, new_prefix)

if __name__ == "__main__":
    # Thay đổi đường dẫn này thành thư mục bạn muốn in (Dấu "." nghĩa là thư mục hiện tại)
    target_dir = Path(".")
    
    print(f"📁 {target_dir.resolve().name}")
    print_tree(target_dir)